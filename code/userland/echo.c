/// Simple program to test whether running a user program works.
///
/// Just do a “syscall” that shuts down the OS.
///
/// NOTE: for some reason, user programs with global data structures
/// sometimes have not worked in the Nachos environment.  So be careful out
/// there!  One option is to allocate data structures as automatics within a
/// procedure, but if you do this, you have to be careful to allocate a big
/// enough stack to hold the automatics!


#include "syscall.h"


int
main(void)
{
	char bf[10];
    //~ Create("test.txt");
    //~ OpenFileId f = Open("test.txt");
    Read(bf,10,0);//Leo de STDIN
    Write(bf,10,1);//Escribo en STDOUT
    //~ Close(o);
    // Not reached.
}