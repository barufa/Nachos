#include "syscall.h"

int
main()
{
    Create("test.txt");
    OpenFileId f = Open("test.txt");
    Write("Hola mundo\n",12,f);
    Close(f);
    Halt();
    return 0;
}
