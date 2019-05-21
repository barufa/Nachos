#include "syscall.h"

int
main(void)
{
    char * argv[] = {"A la grande le puse cuca"};
    SpaceId n = Exec("userland/filetest",1,argv);
    Join(n);
    Halt();
}
