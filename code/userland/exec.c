#include "syscall.h"

int
main(void)
{
    SpaceId n = Exec("userland/filetest");
    Join(n);
    Halt();
}
