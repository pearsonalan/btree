//
// dbg.h
//

#ifdef DEBUG

#ifndef DISABLE_DEBUG_OUT	
#define DISABLE_DEBUG_OUT	0
#endif

#ifndef DEBUG_OUT_TO_STDOUT	
#define DEBUG_OUT_TO_STDOUT	0
#endif

#ifndef DEBUG_OUT_TO_FILE	
#define DEBUG_OUT_TO_FILE	0
#endif

#ifndef DEBUG_OUT_TO_WINDEBUG	
#define DEBUG_OUT_TO_WINDEBUG	0
#endif



namespace dbg {

    class dbgoutbuf : public std::streambuf {
    private:
#if DEBUG_OUT_TO_FILE
	std::ofstream _out;
#endif //DEBUG_OUT_TO_FILE
	typedef std::char_traits<char> traits;
    public:
	dbgoutbuf() {
#if DEBUG_OUT_TO_FILE
	    _out.open( "dbg.out" );
#endif //DEBUG_OUT_TO_FILE
	}
    protected:
	// write one character
	virtual int_type overflow( int_type c ) {
	    if ( ! traits::eq_int_type( c, traits::eof() ) ) {
#if !DISABLE_DEBUG_OUT
		
#if DEBUG_OUT_TO_STDOUT
		fputc( c, stdout );
#endif //DEBUG_OUT_TO_STDOUT

#if DEBUG_OUT_TO_FILE
		if( _out ) {
		    _out.put( c );
		    _out.flush();
		}
#endif //DEBUG_OUT_TO_FILE

#if DEBUG_OUT_TO_WINDEBUG
		char sz[2];
		sz[0] = c;
		sz[1] = 0;
		OutputDebugString(sz);
#endif // DEBUG_OUT_TO_WINDEBUG
		
#endif // !DISABLE_DEBUG_OUT
	    }
	    return traits::not_eof(c);
	}

	virtual std::streamsize xsputn( const char* s, std::streamsize num ) {
#if !DISABLE_DEBUG_OUT
	    
#if DEBUG_OUT_TO_STDOUT
	    fwrite( s, 1, num, stdout );
#endif // DEBUG_OUT_TO_STDOUT

#if DEBUG_OUT_TO_FILE
	    if( _out ) {
		_out.write( s, num );
		_out.flush();
	    }
	    
#endif // DEBUG_OUT_TO_FILE

#if DEBUG_OUT_TO_WINDEBUG
	    std::string str(s,num); // need a string to null-terminate the input buffer
	    OutputDebugString( str.c_str() );
#endif
	    
#endif // !DISABLE_DEBUG_OUT

	    return num;
	}
    };

    
    class nullbuf : public std::streambuf {
    private:
	typedef std::char_traits<char> traits;
    protected:
	virtual int_type overflow( int_type c ) {
	    return traits::not_eof(c);
	}
	virtual std::streamsize xsputn( const char* s, std::streamsize num ) {
	    return num;
	}
    };


    class DebugOutputManager {
    private:
	nullbuf	   _nulbuf;
	dbgoutbuf  _dbgbuf;

	std::ostream _nulout;
	std::ostream _dbgout;

	std::map<std::string,int>	_map;
	
    public:
	DebugOutputManager() ;
	std::ostream& operator () ( const char* cat, int nLevel ) ;
	bool isEnabled( const char* cat, int nLevel ) ;
    };

    enum {
	Error = 0x01,
	Warn = 0x02,
	Info = 0x03,
	Verbose = 0x04,
	Debug = 0x05,
	Debug2 = 0x06,
    };
};

extern dbg::DebugOutputManager dout;

#define DBG(x) x

#else

#define DBG(x) 0

#endif
