// winos.h

namespace os {
    class FileHandle {
    protected:
	HANDLE	_h;

    public:
	FileHandle() throw() {
	    _h = INVALID_HANDLE_VALUE;
	}

	FileHandle( std::string fname, File::OMODE om, File::RDMODE rm, File::SMODE sm, File::AMODE am ) throw( IoException );
	
	~FileHandle() throw() {
	    if( _h != INVALID_HANDLE_VALUE ) {
		::CloseHandle( _h );
		_h = INVALID_HANDLE_VALUE;
	    }
	}

	operator HANDLE () { return _h; }
	HANDLE getHandle() { return _h; }
    };
}

