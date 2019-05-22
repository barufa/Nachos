#include "syscall.h"

int
main(void)
{
    char * argv[] = {"archivo.txt","A la grande le puse cuca"};
    SpaceId n = Exec("userland/filetest",argv);
    Join(n);
    Halt();
}
