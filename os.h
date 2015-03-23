//
// os.h
//

namespace os {

    class IoException : public std::exception {
    protected:
	std::string _error;
    public:
	IoException() {}
	IoException( std::string strMessage );
	IoException( std::string fname, unsigned int nError );
	IoException( std::string strMessage, std::string fname, unsigned int nError );
	~IoException() throw() {}
	virtual const char* what() const throw() ;
    };

    class FileNotFoundException : public IoException {
    public:
	FileNotFoundException( std::string fname ) ;
    };

    class AccessDeniedException : public IoException {
    public:
	AccessDeniedException( std::string fname ) ;
    };
    
    class FileHandle;
    
    class File {
    protected:
	std::string _fname;
	boost::shared_ptr<FileHandle>	_h;
	
    public:

	typedef int64_t POS;
	
	enum OMODE {
	    OpenOrCreate,	// open file if it exists, else create
	    Open,		// open file if it exists, else fail
	    CreateOrTruncate,	// create new file, overwriting if it exists
	    Create		// create new file, fail if it exists
	};

	enum RDMODE {
	    None,
	    ReadOnly,
	    WriteOnly,
	    ReadWrite
	};

	enum AMODE {
	    Sequential,
	    Random
	};

	enum SMODE {
	    ShareNone,
	    ShareRead,
	    ShareWrite
	};

	File() ;
	File( std::string fname, OMODE om, RDMODE rm, SMODE sm, AMODE am ) throw(FileNotFoundException) ;

	void open( std::string fname, OMODE om, RDMODE rm, SMODE sm, AMODE am ) throw(FileNotFoundException) ;

	enum SEEKMODE {
	    SeekAbsolute,
	    SeekRelative
	};
	
	POS seek( POS pos, SEEKMODE sm );
	POS tell();

	void write( void* pvData, unsigned int len ) throw(IoException);
	void read( void* pvData, unsigned int len ) throw(IoException);
    };

    unsigned int getTicks();

    namespace mem {

	inline void clear( void* pv, size_t len ) {
#ifdef WIN32
	    ZeroMemory( pv, len );
#else
	    bzero(pv,len);
#endif
	}

	inline void copy( void* dest, const void* src, size_t len ) {
#ifdef WIN32
	    CopyMemory( dest, src, len );
#else
	    bcopy( src, dest, len );
#endif
	}

	inline void move( void* dest, const void* src, size_t len ) {
#ifdef WIN32
	    MoveMemory( dest, src, len );
#else
	    bcopy( src, dest, len );
#endif
	}
	
    }
}

inline char* strcpy_n( char* d, const char* s, int len ) {
    char* r = d;
    for( ; *s && d - r < len-1; s++, d++ )
	*d = *s;
    *d = '\0';
    return r;
}

#ifndef _STLPORT_VERSION
inline std::ostream& operator << ( std::ostream& os, os::File::POS i ) {
	char sz[40];
	sprintf(sz,"%I64d", i );
	return os << sz;
}

inline std::istream& operator >> ( std::istream& is, os::File::POS& i ) {
	std::string s;
	is >> s;
	sscanf( s.c_str(), "%I64d", &i );
	return is;
}
#endif

#ifdef WIN32
# ifdef _STLPORT_VERSION
#  define USEFACETPARAMS(l,n,b) l
# else
#  if _MSC_VER >= 1300
#    define USEFACETPARAMS(l,n,b) l
#  else
#    define USEFACETPARAMS(l,n,b) l,n,b
#  endif
# endif
#endif

#ifdef UNIX
# define USEFACETPARAMS(l,n,b) l
#endif

// Function class to determine if a given character is a space or not
//
template<class C>
class NotSpace {
    const std::locale loc;
    const std::ctype<C>& ct;
public:
    NotSpace() : loc( std::locale() ), ct(std::use_facet< std::ctype<C> >(USEFACETPARAMS(loc,0,true))) {}
    bool operator() ( C c ) {
	return !ct.is(std::ctype_base::space,c);
    }
};


// destructively trim whitespace from the LEFT (beginning) of a string
//   the given string is modified, and a reference to it is returned
template<class S >
S& ltrim( S& s ) {
#ifdef WIN32    
    s.erase(s.begin(),std::find_if( s.begin(), s.end(), NotSpace<S::value_type>() ));
#else    
    s.erase(s.begin(),std::find_if( s.begin(), s.end(), NotSpace<typename S::value_type>() ));
#endif
    return s;
}

// destructively trim whitespace from the RIGHT (end) of a string
//   the given string is modified, and a reference to it is returned
template<class S>
S& rtrim( S& s ) {
#ifdef WIN32    
    s.erase(std::find_if( s.rbegin(), s.rend(), NotSpace<S::value_type>() ).base(), s.end() );
#else    
    s.erase(std::find_if( s.rbegin(), s.rend(), NotSpace<typename S::value_type>() ).base(), s.end() );
#endif
    return s;
}

// destructively trim whitespace from the BOTH ends of a string
//   the given string is modified, and a reference to it is returned
template<class S>
S& trim( S& s ) {
    ltrim(s);
    rtrim(s);
    return s;
}

namespace boost {
    template <typename T>
    inline std::ostream& operator << ( std::ostream& os, const sub_match<T>& sm ) {
	return os << (std::string)sm;
    }
}


#ifdef WIN32
inline unsigned int random() {
    return (rand() << 16) | rand() ;
}
#endif

