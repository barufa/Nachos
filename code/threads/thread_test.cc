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

#include <unistd.h>

#ifndef THREAD_TEST_TYPE
    #define THREAD_TEST_TYPE SIMPLE
#endif

enum TEST_TYPE{SIMPLE, SEMAPHORE, LOCK, CODITION};

typedef struct arg_sem{
    char * name;
    Semaphore * f;
}arg_sem;

typedef struct arg_lock{
    char * name;
    Lock * l;
    int  * sum;
}arg_lock;

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
        printf("*** Thread `%s` is running: iteration %u\n", currentThread->GetName(), num);
        currentThread->Yield();
    }
    printf("!!! Thread `%s` has finished\n", name);
}

void
SimpleThreadSem(void *_args)
{
    char * name   = ((arg_sem *)_args)->name;
    Semaphore * f = ((arg_sem *)_args)->f;
    f->P();
    DEBUG('s',"Thread `%s` has entered in the critical zone\n",currentThread->GetName());
    for (unsigned num = 0; num <= 100; num++) {
        if(num==10){
            printf("!!! Thread `%s` has finished\n", currentThread->GetName());
        }else{
            printf("*** Thread `%s` is running: iteration %u\n", currentThread->GetName(), num);
            currentThread->Yield();
        }
    }
    DEBUG('s',"Thread `%s` is leaving the critical zone\n",currentThread->GetName());
    f->V();
}

void
SimpleThreadLock(void * args_)
{
    char * name   = ((arg_lock *)args_)->name;
    Lock * l      = ((arg_lock *)args_)->l;
    int  * s      = ((arg_lock *)args_)->sum;

    for (unsigned num = 0; num < 100; num++){
        DEBUG('s',"Thread `%s` is trying to enter in the critical zone\n",name);
        l->Acquire();
        DEBUG('s',"Thread `%s` has entered in the critical zone\n",name);
        printf("*** Thread `%s` is running\n", name);
        (*s)++;
        DEBUG('s',"Thread `%s` is leaving the critical zone\n",name);
        l->Release();
    }
    printf("\t %s finishes with SUM = %d\n",name,*s);

    return;
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching ten threads which call `SimpleThread`, and finally
/// calling `SimpleThread` ourselves.
void
ThreadTest()
{
    DEBUG('t', "Entering thread test\n");

    switch (THREAD_TEST_TYPE) {
        case SEMAPHORE:
            {
                DEBUG('t', "THREAD_TEST_TYPE=SEMAPHORE\n");
                Semaphore *f = new Semaphore("SEMAPHORE_TEST",3);
                for(char i = '1';i<='9';i++){
                  arg_sem * a_sem = new arg_sem;
                  a_sem->f = f;
                  a_sem->name = new char [64];
                  strncpy(a_sem->name, "_nd", 64);
                  a_sem->name[0]=i;
                  Thread *newThread = new Thread(a_sem->name);
                  newThread->Fork(SimpleThreadSem, (void *) a_sem);
                }

                arg_sem * a_sem = new arg_sem;
                a_sem->f = f;
                a_sem->name = new char [64];
                strncpy(a_sem->name, "0st", 64);
                SimpleThreadSem((void *) a_sem);
            }
            break;
        case LOCK:
            {
                DEBUG('t', "THREAD_TEST_TYPE=LOCK\n");
                Lock * l = new Lock("LOCK_TEST");
                // int * sum = new int;
                // *sum = 0;
                int sum = 0;
                for(char i = '1';i<='9';i++){
                  arg_lock * a_lock = new arg_lock;
                  a_lock->l = l;
                  a_lock->name = new char [64];
                  a_lock->sum  = &sum;
                  strncpy(a_lock->name, "_nd", 64);
                  a_lock->name[0]=i;
                  Thread *newThread = new Thread(a_lock->name);
                  newThread->Fork(SimpleThreadLock, (void *) a_lock);
                }
                arg_lock * a_lock = new arg_lock;
                a_lock->l = l;
                a_lock->name = new char [64];
                a_lock->sum  = &sum;
                strncpy(a_lock->name, "0st", 64);
                SimpleThreadLock((void *) a_lock);
            }
            break;
        default:
            {
                DEBUG('t', "THREAD_TEST_TYPE=SIMPLE\n");
                for(char i = '2';i<='5';i++){
                  char *name = new char [64];
                  strncpy(name, "_nd", 64);
                  name[0]=i;
                  Thread *newThread = new Thread(name);
                  newThread->Fork(SimpleThread, (void *) name);
                }

                SimpleThread((void *) "1st");
            }
            break;
    }
}
