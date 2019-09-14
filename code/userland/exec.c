#include "syscall.h"

int
main(void)
{
    char * argv[] = { "friends.txt", "Joey doesn't share food!" };
    SpaceId n     = Exec("userland/filetest", argv, 1);

    Join(n);
    Halt();
}
