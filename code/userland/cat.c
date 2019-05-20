#include "syscall.h"
#define BSIZE 4096

int
main(int arg,char * argv[])
{
	char * filename = argv[1];
    char buffer[BSIZE];
    int bs = 0, i = -1;
    
    OpenFileId f = Open(filename);
	do{
		Read(&buffer[++i],1,f);
	}while(buffer[i]!=ENDCHAR && buffer[i]!='\n');
    Write(buffer,bs,f);
    Close(f);
    Halt();
}
