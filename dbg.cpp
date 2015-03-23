#include "stdinc.h"
#include "os.h"
#include "dbg.h"

#ifdef DEBUG

dbg::DebugOutputManager dout;

namespace dbg {

    DebugOutputManager::DebugOutputManager() : _nulout(&_nulbuf), _dbgout(&_dbgbuf) {
	static boost::regex reStripComments( "(.*)(:?#.*)" );
	static boost::regex reDefinition( "([\\w\\.]+)\\s*:\\s*(\\w+)" );
    
	std::ifstream fs( "dbg.cfg" );
    
	std::string line;
	int lineno = 0;
	
	while( std::getline( fs, line ) ) {
	    ++lineno;
	
	    line = boost::regex_merge( line, reStripComments, "$1" );
	    trim(line);
	    if( line.empty() )
		continue;

	    boost::match_results<std::string::const_iterator> m;
	    
	    if( boost::regex_match( line, m, reDefinition ) ) {
		std::string fac = m[1];
		int nLevel = boost::lexical_cast<int>( m[2] );
		_map.insert( std::make_pair( fac, nLevel ) );
	    }
	}
    }

    std::ostream& DebugOutputManager::operator () ( const char* cat, int nLevel ) {
	if( isEnabled(cat,nLevel) ) {
	    std::ostream& os = _dbgout;
	    os << cat << "[" << (nLevel & 0xFF) << "] ";
	    return os;
	} else
	    return _nulout;
    }

    bool DebugOutputManager::isEnabled( const char* cat, int nLevel ) {
	std::map<std::string,int>::iterator pos = _map.find( std::string(cat) );
	int nCatLevel = 0;
	if( pos != _map.end() ) {
	    nCatLevel = pos->second;
	}
	return (nLevel & 0x000000FF) <= nCatLevel;
    }
}


#endif
