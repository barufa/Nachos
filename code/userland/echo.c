#include "syscall.h"

static inline unsigned
strlen(const char * s)
{
    unsigned i;
    for (i = 0; s[i] != '\0' && s[i] != 0; i++);
    return i;
}

void PutStr(const char * str){
    Write(str, strlen(str), CONSOLE_OUTPUT);
    Write("\n", strlen("\n"), CONSOLE_OUTPUT);
}

int
main(int argc, char * argv[])
{
    int i;
    for(i=1;i<argc;i++){
        PutStr(argv[i]);
    }
    Exit(i);
}
