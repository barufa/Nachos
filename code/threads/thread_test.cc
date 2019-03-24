/// Simple test case for the threads assignment.
///
/// Create several threads, and have them context switch back and forth
/// between themselves by calling `Thread::Yield`, to illustrate the inner
/// workings of the thread system.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "system.hh"
#include "synch.hh"


typedef struct arg_sem{
    char * name;
    Semaphore * f;
}arg_sem;

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
void
SimpleThread(void *name_)
{
    // Reinterpret arg `name` as a string.
    char *name = (char *) name_;

    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++) {
        printf("*** Thread `%s` is running: iteration %u\n", name, num);
        currentThread->Yield();
    }
    printf("!!! Thread `%s` has finished\n", name);
}

void
SimpleThreadSem(void *_args)
{
    char * name   = ((arg_sem *)_args)->name;
    Semaphore * f = ((arg_sem *)_args)->f;
    for (unsigned num = 0; num <= 10; num++) {
        f->P();
        DEBUG('s',"Thread `%s` has entered in the critical zone\n",name);
        if(num==10){
            printf("!!! Thread `%s` has finished\n", name);
        }else{
            printf("*** Thread `%s` is running: iteration %u\n", name, num);
            currentThread->Yield();   
        }
        DEBUG('s',"Thread `%s` is leaving the critical zone\n",name);
        f->V();
    }
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching ten threads which call `SimpleThread`, and finally
/// calling `SimpleThread` ourselves.
void
ThreadTest()
{
    DEBUG('t', "Entering thread test\n");
#ifdef SEMAPHORE_TEST
    Semaphore *f = new Semaphore("SEMAPHORE_TEST",3);
    for(char i = '2';i<='5';i++){
      arg_sem * argumentos = new arg_sem;
      argumentos->f = f;
      argumentos->name = new char [64];
      strncpy(argumentos->name, "_nd", 64);
      argumentos->name[0]=i;
      Thread *newThread = new Thread(argumentos->name);
      newThread->Fork(SimpleThreadSem, (void *) argumentos);
    }

    arg_sem * argumentos = new arg_sem;
    argumentos->f = f;
    argumentos->name = new char [64];
    strncpy(argumentos->name, "1st", 64);
    SimpleThreadSem((void *) argumentos);

#else
    for(char i = '2';i<='5';i++){
      char *name = new char [64];
      strncpy(name, "_nd", 64);
      name[0]=i;
      Thread *newThread = new Thread(name);
      newThread->Fork(SimpleThread, (void *) name);
    }

    SimpleThread((void *) "1st");
#endif
}
