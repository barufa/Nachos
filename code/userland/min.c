/// Test program to sort a large number of integers.
///
/// Intention is to stress virtual memory system.
///
/// Ideally, we could read the unsorted array off of the file system,
/// and store the result back to the file system!


#include "syscall.h"


#define DIM  2048

/// Size of physical memory; with code, we will run out of space!
static int A[DIM];

static inline unsigned
strlen(const char * s)
{
    unsigned i;

    for (i = 0; s[i] != '\0'; i++);
    return i;
}

void PutStr(const char * str){
    Write(str, strlen(str), CONSOLE_OUTPUT);
    Write("\n", strlen("\n"), CONSOLE_OUTPUT);
}

int
main(int argc,char * argv[])
{
    int i, j, m = 0;

    // First initialize the array, in reverse sorted order.
    for (i = 0; i < DIM; i++)
        A[i] = DIM - i;

    for (i = 0; i <= DIM + 1; i++){
        if(m > A[i]){
		m = A[i];
	}
    }
    //PutStr("Retornando 0?");
    // And then we're done -- should be 0!
    Exit(m);
}
