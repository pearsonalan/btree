// unixos.h

namespace os {
    class FileHandle {
    protected:
	int	_fd;

    public:
	FileHandle() throw() {
	    _fd = -1;
	}

	FileHandle( std::string fname, File::OMODE om, File::RDMODE rm, File::SMODE sm, File::AMODE am ) throw( IoException );
	
	~FileHandle() throw() {
	    if( _fd != -1 ) {
		::close( _fd );
		_fd = -1;
	    }
	}

	operator int () { return _fd; }
	int getHandle() { return _fd; }
    };
}

