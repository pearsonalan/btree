#include "stdinc.h"
#include "os.h"
#include "winos.h"

namespace os {
    FileHandle::FileHandle( std::string fname, File::OMODE om, File::RDMODE rm, File::SMODE sm, File::AMODE am ) {
	DWORD dwDesiredAccess = 0;
	switch( rm ) {
	case File::ReadOnly:	dwDesiredAccess = GENERIC_READ;			break;
	case File::ReadWrite:	dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;	break;
	case File::WriteOnly:	dwDesiredAccess = GENERIC_WRITE;		break;
	}

	DWORD dwShareMode = 0;
	switch( sm ) {
	case File::ShareNone:	dwShareMode = 0;			break;
	case File::ShareRead:   dwShareMode = FILE_SHARE_READ;		break;
	case File::ShareWrite:	dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE; break;
	}

	DWORD dwCreationDisposition = 0;
	switch( om ) {
	case File::OpenOrCreate:	dwCreationDisposition = OPEN_ALWAYS;		break;
	case File::Open:		dwCreationDisposition = OPEN_EXISTING;		break;
	case File::CreateOrTruncate:	dwCreationDisposition = CREATE_ALWAYS;		break;
	case File::Create:		dwCreationDisposition = CREATE_NEW;		break;
	}

	DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;
	switch( am ) {
	case File::Random:		dwFlags |= FILE_FLAG_RANDOM_ACCESS;	break;
	case File::Sequential:		dwFlags |= FILE_FLAG_SEQUENTIAL_SCAN;	break;
	}

	_h = ::CreateFile( fname.c_str(),
			   dwDesiredAccess,
			   dwShareMode,
			   NULL,
			   dwCreationDisposition,
			   dwFlags,
			   NULL );

	if( _h == INVALID_HANDLE_VALUE ) {
	    DWORD dwError = ::GetLastError() ;

	    switch( dwError ) {
	    case ERROR_ACCESS_DENIED:
		throw AccessDeniedException( fname );

	    case ERROR_FILE_NOT_FOUND:
	    case ERROR_PATH_NOT_FOUND:
		throw FileNotFoundException( fname );

	    default:
		throw IoException( fname, dwError );
	    }
	}
    }

    
    File::POS File::seek( File::POS pos, File::SEEKMODE sm ) {
	if( ! _h || _h->getHandle() == INVALID_HANDLE_VALUE )
	    throw IoException( "Invalid file handle" );

	LONG lDistanceToMove;
	LONG lDistanceToMoveHigh;
	DWORD dwMoveMethod;

	lDistanceToMove = pos & 0xFFFFFFFF;
	lDistanceToMoveHigh = pos >> 32;

	switch( sm ) {
	case SeekAbsolute:
	    dwMoveMethod = FILE_BEGIN;
	    break;

	case SeekRelative:
	    dwMoveMethod = FILE_CURRENT;
	    break;
	}

	// clear any previous error
	SetLastError(NO_ERROR);
	
	DWORD dwResult = ::SetFilePointer( _h->getHandle(),
					   lDistanceToMove,
					   &lDistanceToMoveHigh,
					   dwMoveMethod );

	DWORD dwError;
	if( (dwError = GetLastError()) != NO_ERROR ) {
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
	    oss << " position " << pos ;
	    throw IoException( oss.str(), _fname, dwError );
	}

	POS p = (((POS)lDistanceToMoveHigh) << 32) & dwResult;
	return p;
    }
    
    File::POS File::tell() {
	if( ! _h || _h->getHandle() == INVALID_HANDLE_VALUE )
	    throw IoException( "Invalid file handle" );

	POS p = 0;
	return p;
    }

    void File::write( void* pvData, unsigned int len ) throw(IoException) {
	if( ! _h || _h->getHandle() == INVALID_HANDLE_VALUE )
	    throw IoException( "Invalid file handle" );

	DWORD dwBytesWritten = 0;
	if( ! ::WriteFile( _h->getHandle(),
			   pvData,
			   len,
			   &dwBytesWritten,
			   NULL ) ||
	    dwBytesWritten == 0 )
	{
	    DWORD dwError = ::GetLastError();
	    throw IoException( "write failed", _fname, dwError );
	}
    }

    void File::read( void* pvData, unsigned int len ) throw(IoException) {
	if( ! _h || _h->getHandle() == INVALID_HANDLE_VALUE )
	    throw IoException( "Invalid file handle" );

	DWORD dwBytesRead = 0;
	if( ! ::ReadFile( _h->getHandle(),
			   pvData,
			   len,
			   &dwBytesRead,
			   NULL ) ||
	    dwBytesRead != len )
	{
	    DWORD dwError = ::GetLastError();
	    throw IoException( "read failed", _fname, dwError );
	}
    }

    unsigned int getTicks() {
	return ::GetTickCount();
    }
}
