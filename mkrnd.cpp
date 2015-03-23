#include "stdinc.h"
#include "os.h"

char dat[52];

int main( void ) {
    std::ofstream fs( "rnd.txt" );
    int i;
    
    for( i = 0 ; i < 26; i++ ) 
	dat[i] = 'a' + i ;
    for( i = 0 ; i < 26; i++ ) 
	dat[i+26] = 'A' + i ;
    
    for( i = 0; i < 1024*1024; i++ ) {
	fs << dat[rand() % sizeof(dat)];
    }

    return 0;
}
	
