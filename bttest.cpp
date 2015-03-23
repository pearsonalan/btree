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
	static const int count = 2000;
	
	std::cout << "Opening test.dat" << std::endl;
	bt::BTree bt;
	bt.create( std::string( "test.dat" ) );

	std::vector< std::pair<int,std::string> > values;
	values.reserve(count);

	std::cout << "generating random data." << std::endl;
	
	for( i = 0; i < count; i++ ) {
	    int len;

	    int r = rand();

	    if( r % 100 == 0 )
		len = (rand() % 4000) + 2000;
	    else if( r % 20 == 0 )
		len = (rand() % 2000) + 1000;
	    else if( r % 4 == 0 ) 
		len = (rand()%500)+10;
	    else
		len = 20;
	    
	    for( j = 0; j < len; j++ ) 
		sz[j] = (rand() % 26) + 'a';
	    sz[j] = 0;
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

	    if( (i % ((count/100)+1) ) == 0 ) {
		int p = (i*100)/count;
		if( p != pct ) {
		    pct = p;
		    std::cout << pct << "%\r" << std::flush;
		}
	    }
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


	//
	// test deletion of ~10% of the items
	//

	// keep track of which items are deleted using a bitset
	std::bitset<count> deleted;
	
	std::cout << "beginning deletion." << std::endl;
	
	start = os::getTicks();

	for( i = 0; i < count/10; i++ ) {
	    int n;
	    n = random() % values.size();
	    deleted.set(n);

	    bt.remove( n );
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
