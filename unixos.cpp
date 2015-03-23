#include "stdinc.h"
#include "os.h"
#include "unixos.h"

namespace os {
    FileHandle::FileHandle( std::string fname, File::OMODE om, File::RDMODE rm, File::SMODE sm, File::AMODE am ) throw(os::IoException) {
	int oflag = 0;
	switch( rm ) {
	case File::ReadOnly:	oflag |= O_RDONLY;	break;
	case File::ReadWrite:	oflag |= O_RDWR;	break;
	case File::WriteOnly:	oflag |= O_WRONLY;	break;
	}

	switch( om ) {
	case File::OpenOrCreate:	oflag |= O_CREAT;		break;
	case File::Open:		oflag |= 0;			break;
	case File::CreateOrTruncate:	oflag |= O_CREAT | O_TRUNC;	break;
	case File::Create:		oflag |= O_CREAT | O_EXCL;	break;
	}

	_fd = ::open( fname.c_str(),
		      oflag,
		      0644 );

	if( _fd == -1 ) {
	    int nError = errno ;

	    switch( nError ) {
	    case EACCES:
		throw AccessDeniedException( fname );

	    case ENOENT:
		throw FileNotFoundException( fname );

	    default:
		throw IoException( fname, nError );
	    }
	}
    }

    
    File::POS File::seek( File::POS pos, File::SEEKMODE sm ) {
	if( ! _h || _h->getHandle() == -1 )
	    throw IoException( "Invalid file handle" );

	off_t offset;
	int whence;

	// don't support 64-bit files here...
	offset = pos & 0xFFFFFFFF;

	switch( sm ) {
	case SeekAbsolute:
	    whence = SEEK_SET;
	    break;

	case SeekRelative:
	    whence = SEEK_CUR;
	    break;
	}

	off_t result = ::lseek( _h->getHandle(),
				offset,
				whence );

	if( result == -1 ) {
	    int nError = errno;

	    std::ostringstream oss ;
	    oss << "Cannot seek to " ;
	    switch(sm) {
	    case SeekAbsolute:
		oss << "absolute";
		break;

	    case SeekRelative:
		oss << "relative";
		break;
	    }
	    oss << " position " << (int) pos ;
	    throw IoException( oss.str(), _fname, nError );
	}

	POS p = result;
	return p;
    }
    
    File::POS File::tell() {
	if( ! _h || _h->getHandle() == -1 )
	    throw IoException( "Invalid file handle" );

	POS p = 0;
	return p;
    }

    void File::write( void* pvData, unsigned int len ) throw(IoException) {
	if( ! _h || _h->getHandle() == -1 )
	    throw IoException( "Invalid file handle" );

	ssize_t bytes_written ;
	if( (bytes_written = ::write( _h->getHandle(),
				      pvData,
				      len )) == -1 )
	{
	    int nError = errno;
	    throw IoException( "write failed", _fname, nError );
	}
    }

    void File::read( void* pvData, unsigned int len ) throw(IoException) {
	if( ! _h || _h->getHandle() == -1 )
	    throw IoException( "Invalid file handle" );

	ssize_t bytes_read = 0;
	if( (bytes_read = ::read( _h->getHandle(),
				  pvData,
				  len )) == -1 )
	{
	    int nError = errno;
	    throw IoException( "read failed", _fname, nError );
	}
    }

    unsigned int getTicks() {
	struct timespec tp;
	clock_gettime( CLOCK_HIGHRES, &tp );
	unsigned int ms = tp.tv_sec * 1000 + tp.tv_nsec/1000000;
	return ms;
    }
}
