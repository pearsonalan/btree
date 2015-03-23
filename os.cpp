#include "stdinc.h"
#include "os.h"
#include "dbg.h"

#ifdef WIN32
#include "winos.h"
#endif

#ifdef UNIX
#include "unixos.h"
#endif

namespace os {
    IoException::IoException( std::string strMessage ) {
	_error = strMessage;
    }
    
    IoException::IoException( std::string fname, unsigned int nError ) {
	_error = std::string("I/O exception (error ") + boost::lexical_cast<std::string>(nError) + std::string( ") accessing ") + fname;
    }

    IoException::IoException( std::string message, std::string fname, unsigned int nError ) {
	_error = message + std::string(" (error ") + boost::lexical_cast<std::string>(nError) + std::string( ") accessing ") + fname;
    }

    
    const char* IoException::what() const throw() {
	return _error.c_str();
    }

    FileNotFoundException::FileNotFoundException( std::string fname ) {
	_error = fname + std::string(" not found");
    }
    
    AccessDeniedException::AccessDeniedException( std::string fname ) {
	_error = fname + std::string(": access deined");
    }
    

    File::File() {
    }

    File::File( std::string fname, OMODE om, RDMODE rm, SMODE sm, AMODE am ) throw(FileNotFoundException) {
	open(fname,om,rm,sm,am);
    }

    void File::open( std::string fname, OMODE om, RDMODE rm, SMODE sm, AMODE am ) throw(FileNotFoundException) {
	DBG( dout("os",2) << "Opening " << fname << std::endl );
	_fname = fname;
	_h = boost::shared_ptr<FileHandle>( new FileHandle(fname,om,rm,sm,am) );
    }
}

