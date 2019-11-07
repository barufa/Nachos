#include "syscall.h"

static inline unsigned
strlen(const char * s)
{
    unsigned i;

    for (i = 0; s[i] != '\0'; i++);
    return i;
}

int
main()
{
    char * filename    = "A.txt";
    char * filecontent = "Hola Mundo";

    Create(filename);
    OpenFileId o = Open(filename);
    Write(filecontent, strlen(filecontent), o);
    Close(o);
}
