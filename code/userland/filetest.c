#include "syscall.h"

unsigned
strlen(const char *s){
    unsigned i;
    for (i = 0; s[i] != '\0' && s[i]!='\n'; i++);
    return i;
}

int
main(int argc,char ** argv)
{
    char * filename = argv[1];
    char * filecontent = argv[2];
    Create(filename);
    OpenFileId o = Open(filename);
    Write(filecontent,strlen(filecontent),o);
    Close(o);
}
