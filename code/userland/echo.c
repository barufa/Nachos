#include "syscall.h"
#define BSIZE 30

static inline unsigned
strlen(const char *s)
{
    unsigned i;
    for (i = 0; s[i] != '\0'; i++);
    return i;
}

int
main(int argc,char * argv[])
{
    Write(argv[1],strlen(argv[1]),CONSOLE_OUTPUT);
    Write("\n",1,CONSOLE_OUTPUT);
}
