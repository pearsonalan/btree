#include "stdinc.h"
#include <bitset>
#include "os.h"
#include "dbg.h"
#include "bt.h"

char sz[8192];

class TestException : public std::exception {
protected:
    std::string _error;
public:
    TestException( std::string error ) : _error(error) {}
    virtual ~TestException() throw() {}
    virtual const char* what() const throw() {
	return _error.c_str();
    }
};

int main( void ) {
    try {
	int i, j;
	static const int count = 26;
	
	std::cout << "Opening test.dat" << std::endl;
	bt::BTree bt;
	bt.create( std::string( "test.dat" ) );

	std::vector< std::pair<int,std::string> > values;
	values.reserve(count);

	std::cout << "generating random data." << std::endl;
	
	for( i = 0; i < 26; i++ ) {
	    sz[0] = 'a'+i;
	    sz[1] = 0;
	    values.push_back( std::make_pair( i, std::string(sz) ) );
	}

	int indices[count];
	std::bitset<count> used;
	
	std::cout << "re-ordering insertion of data." << std::endl;

	int pct = 0;
	
	for( i = 0; i < count; i++ ) {
	    int n;
	    do {
		n = random() % values.size();
	    } while( used.test(n) );
	    used.set(n);
	    indices[i] = n;
	}
	


	//
	// test insertion
	//
	
	std::cout << "beginning insertion." << std::endl;
	
	unsigned int start, end;
	
	start = os::getTicks();
	
	for( i = 0; i < count; i++ ) {
	    std::pair<int,std::string> data = values[ indices[i] ];
	    bt.insert( data.first, data.second.c_str(), data.second.length()+1 );
	}

	end = os::getTicks();

	std::cout << "insertion took " << (end-start) << "ms." << std::endl;

	bt.dump();

	//
	// test deletion of ~50% of the items
	//

	// keep track of which items are deleted using a bitset
	std::bitset<count> deleted;
	
	std::cout << "beginning deletion." << std::endl;
	
	start = os::getTicks();

	for( i = 0; i < count/2; i++ ) {
	    int n;
	    n = random() % values.size();
	    deleted.set(n);

	    std::cout << "removing " << std::string(1,'a'+n) << "." << std::endl;
	    bt.remove( n );
	    bt.dump();
	    std::cout << std::endl;
	}
	
	end = os::getTicks();

	std::cout << "deletion took " << (end-start) << "ms." << std::endl;

	
	//
	// test retrieval
	//
	
	start = os::getTicks();

	for( i = 0; i < count; i++ ) {
	    std::pair<int,std::string> data = values[ indices[i] ];

	    if( bt.search( data.first, sz ) ) {
		// item found
		
		// was it deleted?
		if( deleted.test( data.first ) )
		    throw TestException( "search incorrectly returned deleted item" );
		if( data.second != sz )
		    throw TestException( "search did not match correct item" );
	    } else {
		// item not found
		
		// was it deleted?
		if( !deleted.test( data.first ) )
		    throw TestException( "search failed to find expected item" );
	    }
	}
	
	end = os::getTicks();

	std::cout << "retrieval took " << (end-start) << "ms." << std::endl;
	    
    } catch( std::exception& x ) {
	std::cout << "Error: " << x.what() << std::endl;
	return -1;
    }
    
    return 0; 
}
