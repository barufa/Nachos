#include "syscall.h"

int
main(void)
{
    char * argv[] = {"archivo.txt","Hello world"};
    SpaceId n = Exec("userland/filetest",argv);
    Join(n);
    Halt();
}
