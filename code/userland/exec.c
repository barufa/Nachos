#include "syscall.h"

int
main(void)
{
    char * argv[] = {"userland/filetest","Friends.txt", "Joe doesn't share food!", NULL};
    SpaceId n     = Exec("userland/filetest", argv, 1);

    Join(n);
    Exit(0);
}
