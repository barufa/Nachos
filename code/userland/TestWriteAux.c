#include "syscall.h"

static inline unsigned
strlen(const char * s)
{
    unsigned i;

    for (i = 0; s[i] != '\0'; i++);
    return i;
}

int
main(void)
{
    int i, j;
    OpenFileId id = Open("File.txt");

    Write("Start Writer 2\n", strlen("Start Writer 2\n"), CONSOLE_OUTPUT);
    for (i = 0; i < 20; i++) {
        Write("Test Writer 2 \n", strlen("Test Writer 2 \n"), id);
        for (j = 0; j < 1000; j++);
    }
    Write("Finish Writer 2\n", strlen("Finish Writer 2\n"), CONSOLE_OUTPUT);
    Close(id);
}
