#ifdef WIN32

#pragma warning( disable : 4786 )

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <conio.h>
#include <ctype.h>

typedef __int64		int64_t;

#endif

#ifdef UNIX

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#endif
