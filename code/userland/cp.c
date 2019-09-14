#include "syscall.h"
#define BSIZE 4096

int
main(int arg, char * argv[])
{
    char * src = argv[1], * dst = argv[2];
    char buffer[BSIZE];
    int bs = 0, i = -1;

    Create(dst);
    OpenFileId Fsrc = Open(src), Fdst = Open(dst);

    do {
        Read(&buffer[++i], 1, Fsrc);
    } while (buffer[i] != ENDCHAR && buffer[i] != '\n');
    Write(buffer, i, Fdst);

    Close(Fsrc);
    Close(Fdst);
    Halt();
}
