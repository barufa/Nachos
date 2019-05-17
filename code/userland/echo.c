#include "syscall.h"
#define BSIZE 4096

int
main(void)
{
	char bf[BSIZE];
	int i=-1;
	do{
		Read(&bf[++i],1,CONSOLE_INPUT);
	}while(bf[i]!='\n' && bf[i]!=ENDCHAR);
    Write(bf,i,CONSOLE_OUTPUT);
    Halt();
}
