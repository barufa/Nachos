#include "syscall.h"
#define BSIZE 30

int
main(int arg, char * argv[])
{
    int i = 0;

    do {
        Write("Tik\n-->", 7, CONSOLE_OUTPUT);
        for (i = 0; i < 7000; i++);
    } while (1);
    Exit(0);
}
