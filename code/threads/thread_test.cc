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

#define ATOM(l, p) { l->Acquire(), p, l->Release(); }

#ifndef THREAD_TEST_TYPE
# define THREAD_TEST_TYPE SIMPLE
#endif

enum TEST_TYPE { SIMPLE, SEMAPHORE, LOCK, CONDITION, PORT, JOIN };

typedef struct arg_lock {
    Lock * l;
    int *  sum;
} arg_lock;

typedef struct arg_cond {
    Lock *      l;
    Condition * c;
    int *       n;
} arg_cond;

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
void
SimpleThread(void * name_)
{
    // Reinterpret arg `name` as a string.
    char * name = (char *) name_;

    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++) {
        printf("*** Thread `%s` is running: iteration %u\n",
          currentThread->GetName(), num);
        currentThread->Yield();
    }
    printf("!!! Thread `%s` stops\n", name);
}

void
SimpleThreadSem(void * _args)
{
    const char * name = currentThread->GetName();
    Semaphore * f     = (Semaphore *) _args;

    f->P();
    DEBUG('s', "Thread `%s` has entered in the critical zone\n", name);
    for (unsigned num = 0; num <= 10; num++) {
        if (num == 10) {
            printf("!!! Thread `%s` has finished\n", name);
        } else {
            printf("*** Thread `%s` is running: iteration %u\n", name, num);
            currentThread->Yield();
        }
    }
    DEBUG('s', "Thread `%s` is leaving the critical zone\n", name);
    f->V();
}

void
SimpleThreadLock(void * args_)
{
    const char * name = currentThread->GetName();
    Lock * l = ((arg_lock *) args_)->l;
    int * s  = ((arg_lock *) args_)->sum;

    for (unsigned num = 0; num < 100; num++) {
        DEBUG('s', "Thread `%s` is trying to enter in the critical zone\n",
          name);
        l->Acquire();
        DEBUG('s', "Thread `%s` has entered in the critical zone\n", name);
        printf("*** Thread `%s` is running\n", name);
        (*s)++;
        DEBUG('s', "Thread `%s` is leaving the critical zone\n", name);
        l->Release();
    }
    printf("\t %s finishes with SUM = %d\n", name, *s);
}

void
SimpleThreadCond(void * args_)
{
    const char * name = currentThread->GetName();
    arg_cond * ar     = (arg_cond *) args_;
    Condition * c     = ar->c;
    Lock * l = ar->l;
    int * n  = ar->n;

    for (unsigned num = 1; num <= 20; num++) {
        if ((name[0] - '0') % 2) {// Consumo 1
            l->Acquire();
            while (*n <= 0) {
                c->Wait();
            }
            printf("Thread `%s` eates 1 %d\n", name, *n);
            (*n)--;
            l->Release();
        } else if (num % 5 != 0) { // Creo 1
            l->Acquire();
            (*n)++;
            printf("Thread `%s` creates 1 %d\n", name, *n);// *n volatil, solo para ver
            l->Release();
            c->Signal();
        } else { // Creo 5
            l->Acquire();
            (*n) += 5;
            printf("Thread `%s` creates 5 %d\n", name, *n);
            l->Release();
            c->Broadcast();
        }
    }
    printf("Finalizo %s con N=%d\n", name, *n);
}

void
SimpleThreadPort(void * args_)
{
    const char * name = currentThread->GetName();
    Port * p = (Port *) args_;

    for (int num = 0; num < 5; num++) {
        if ((name[0] - '0') % 2) {// Receptor
            int m = 0;
            p->Receive(&m);
            printf("**Thread `%s` recieves %d\n", name, m);
        } else {// Emisor
            p->Send(num + 1000);
            printf("**Thread `%s` sends %d\n", name, num + 1000);
        }
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

    switch (THREAD_TEST_TYPE) {
        case SEMAPHORE: {
            DEBUG('t', "THREAD_TEST_TYPE=SEMAPHORE\n");
            Semaphore * f = new Semaphore("SEMAPHORE_TEST", 3);
            for (char i = '1'; i <= '9'; i++) {
                char * name = new char [64];
                strncpy(name, "_nd", 64);
                name[0] = i;
                Thread * newThread = new Thread(name);
                newThread->Fork(SimpleThreadSem, (void *) f);
            }
            SimpleThreadSem((void *) f);
        }
        break;
        case LOCK: {
            DEBUG('t', "THREAD_TEST_TYPE=LOCK\n");
            Lock * l  = new Lock("LOCK_TEST");
            int * sum = new int;
            *sum = 0;
            for (char i = '1'; i <= '9'; i++) {
                arg_lock * a_lock = new arg_lock;
                a_lock->l   = l;
                a_lock->sum = sum;
                char * name = new char [64];
                strncpy(name, "_nd", 64);
                name[0] = i;
                Thread * newThread = new Thread(name);
                newThread->Fork(SimpleThreadLock, (void *) a_lock);
            }
            arg_lock * a_lock = new arg_lock;
            a_lock->l = l;
            char * name = new char [64];
            a_lock->sum = sum;
            strncpy(name, "0st", 64);
            SimpleThreadLock((void *) a_lock);
        }
        break;
        case CONDITION: {
            DEBUG('t', "THREAD_TEST_TYPE=CONDITION\n");
            Lock * l      = new Lock("LOCK_TEST");
            Condition * c = new Condition("COND_TEST", l);
            int * n       = new int;
            *n = 0;
            for (char i = '1'; i <= '4'; i++) {
                char * name = new char [64];
                strncpy(name, "_nd", 64);
                name[0] = i;
                Thread * newThread = new Thread(name);
                arg_cond * a_cond  = new arg_cond;
                a_cond->l = l;
                a_cond->c = c;
                a_cond->n = n;
                newThread->Fork(SimpleThreadCond, (void *) a_cond);
            }
            arg_cond * a_cond = new arg_cond;
            a_cond->l = l;
            a_cond->c = c;
            a_cond->n = n;
            SimpleThreadCond((void *) a_cond);
        }
        break;
        case PORT: {
            DEBUG('t', "THREAD_TEST_TYPE=PORT\n");
            Port * p = new Port("Port Test");
            for (char i = '1'; i <= '9'; i++) {
                char * name = new char [64];
                strncpy(name, "_nd", 64);
                name[0] = i;
                Thread * newThread = new Thread(name);
                newThread->Fork(SimpleThreadPort, (void *) p);
            }
            SimpleThreadPort((void *) p);
        }
        break;
        case JOIN: {
            DEBUG('t', "THREAD_TEST_TYPE=JOIN\n");

            Thread * threads_arr[20];

            for (int i = 1; i <= 9; i++) {
                char * name = new char [64];
                strncpy(name, "_nd", 64);
                name[0]        = '0' + i;
                threads_arr[i] = new Thread(name, true);
                threads_arr[i]->Fork(SimpleThread, (void *) name);
            }

            for (int i = 1; i <= 9; i++) {
                printf("Esperando a %d\n", i);
                threads_arr[i]->Join();
                printf("Finalizo %d\n", i);
            }

            printf("=====%s puede terminar=====\n", currentThread->GetName());
        }
        break;
        default: {
            DEBUG('t', "THREAD_TEST_TYPE=SIMPLE\n");
            for (char i = '2'; i <= '5'; i++) {
                char * name = new char [64];
                strncpy(name, "_nd", 64);
                name[0] = i;
                Thread * newThread = new Thread(name);
                newThread->Fork(SimpleThread, (void *) name);
            }

            SimpleThread((void *) "1st");
        }
        break;
    }
} // ThreadTest
