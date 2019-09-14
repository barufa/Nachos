#include "syscall.h"

int
main()
{
    char * filename    = "A.txt";
    char * filecontent = "Hola\n";

    Create(filename);
    OpenFileId o = Open(filename);
    Write(filecontent, 4, o);
    Close(o);
    Halt();
}
