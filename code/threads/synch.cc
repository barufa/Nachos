/// Routines for synchronizing threads.
///
/// Three kinds of synchronization routines are defined here: semaphores,
/// locks and condition variables (the implementation of the last two are
/// left to the reader).
///
/// Any implementation of a synchronization routine needs some primitive
/// atomic operation.  We assume Nachos is running on a uniprocessor, and
/// thus atomicity can be provided by turning off interrupts.  While
/// interrupts are disabled, no context switch can occur, and thus the
/// current thread is guaranteed to hold the CPU throughout, until interrupts
/// are reenabled.
///
/// Because some of these routines might be called with interrupts already
/// disabled (`Semaphore::V` for one), instead of turning on interrupts at
/// the end of the atomic operation, we always simply re-set the interrupt
/// state back to its original value (whether that be disabled or enabled).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2018 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "synch.hh"
#include "system.hh"


/// Initialize a semaphore, so that it can be used for synchronization.
///
/// * `debugName` is an arbitrary name, useful for debugging.
/// * `initialValue` is the initial value of the semaphore.
Semaphore::Semaphore(const char *debugName, int initialValue)
{
    name  = debugName;
    value = initialValue;
    queue = new List<Thread *>;

}

/// De-allocate semaphore, when no longer needed.
///
/// Assume no one is still waiting on the semaphore!
Semaphore::~Semaphore()
{
    delete queue;
}

const char *
Semaphore::GetName() const
{
    return name;
}

/// Wait until semaphore `value > 0`, then decrement.
///
/// Checking the value and decrementing must be done atomically, so we need
/// to disable interrupts before checking the value.
///
/// Note that `Thread::Sleep` assumes that interrupts are disabled when it is
/// called.
void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);
      // Disable interrupts.
    DEBUG('s',"%s hace P en %s\n",currentThread->GetName(),name);
    while (value == 0) {  // Semaphore not available.
        queue->Append(currentThread);  // So go to sleep.
        currentThread->Sleep();
    }
    value--;  // Semaphore available, consume its value.

    interrupt->SetLevel(oldLevel);  // Re-enable interrupts.
}

/// Increment semaphore value, waking up a waiter if necessary.
///
/// As with `P`, this operation must be atomic, so we need to disable
/// interrupts.  `Scheduler::ReadyToRun` assumes that threads are disabled
/// when it is called.
void
Semaphore::V()
{
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);
    DEBUG('s',"%s hace V en %s\n",currentThread->GetName(),name);

    Thread *thread = queue->Pop();
    if (thread != nullptr)
        // Make thread ready, consuming the `V` immediately.
        scheduler->ReadyToRun(thread);
    value++;

    interrupt->SetLevel(oldLevel);
}

/// Dummy functions -- so we can compile our later assignments.
///
/// Note -- without a correct implementation of `Condition::Wait`, the test
/// case in the network assignment will not work!

Lock::Lock(const char *debugName)
{
    name = debugName;
    lock = new Semaphore(name,1);
    thread = NULL;
}

Lock::~Lock()
{
    delete lock;
}

const char *
Lock::GetName() const
{
    return name;
}

void
Lock::Acquire()
{
    ASSERT(!IsHeldByCurrentThread());

    lock->P();
    thread = currentThread;
    DEBUG('t',"%s acquires the lock %s\n",thread->GetName(),name);
}

void
Lock::Release()
{
    ASSERT(IsHeldByCurrentThread());
    DEBUG('t',"%s releases the lock %s\n",thread->GetName(),name);
    thread = NULL;
    lock->V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    return thread==currentThread;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

Condition::Condition(const char *debugName, Lock *conditionLock)
{
    name = debugName;
    condition = conditionLock;
    internal = new Lock("Internal_Lock_Condition");
    q_threads = new List<Semaphore *>;
}

Condition::~Condition()
{
    delete internal;
    delete q_threads;
}

const char *
Condition::GetName() const
{
    return name;
}

void
Condition::Wait()
{
    ASSERT(condition->IsHeldByCurrentThread());
    Semaphore  *f = new Semaphore("Semaforo_de_cond",0);
    q_threads->Append(f);
    condition->Release();
    f->P();
    condition->Acquire();
}

void
Condition::Signal()
{
    internal->Acquire();
    if(!q_threads->IsEmpty()){
        Semaphore * f = q_threads->Pop();//Borra elemento
        f->V();
    }
    internal->Release();
}

void
Condition::Broadcast()
{
    internal->Acquire();
    while(!q_threads->IsEmpty()){
        Semaphore * f = q_threads->Pop();
        f->V();
    }
    internal->Release();
}


/////////////////////////////////////////////////
/////////////////////////////////////////////////

Port::Port(const char *debugName)
{
    name=debugName;
    internal_lock = new Lock("Internal_port_lock");
    cond_new_receiver = new Condition("Receiver_condition", internal_lock);
    cond_message = new Condition("Message_condition", internal_lock);
    buffer_flag = false; //true=>buffer not empty
    buffer = 0;
    num_receive = 0;
}

Port::~Port()
{
    delete cond_new_receiver;
    delete cond_message;
    delete internal_lock;
}

const char *
Port::GetName() const
{
    return name;
}

void
Port::Send(int msg)
{
    DEBUG('t', "%s: The thread %s wants to send: %d\n",name, currentThread->GetName(), msg);

    internal_lock->Acquire();

    //si el buffer esta null, espero a que se llame un receive
    while(num_receive<=0)
    {
        DEBUG('t', "%s: The thread %s is waiting for a receiver\n",name,currentThread->GetName());
        cond_new_receiver->Wait();
    }

    buffer = msg;
    buffer_flag = true;
    cond_message->Signal();
    DEBUG('t', "%s: The thread %s sent: %d\n",name, currentThread->GetName(), msg);

    internal_lock->Release();
}

void
Port::Receive(int *msg)
{
    internal_lock->Acquire();
    DEBUG('t', "%s: The thread %s wants to receive a message\n",name, currentThread->GetName());

    num_receive++;
    cond_new_receiver->Signal();

    while(!buffer_flag)
    {
        DEBUG('t', "%s: The thread %s is waiting for a message\n",name,currentThread->GetName());
        cond_message->Wait();
    }

    *msg = buffer;
    buffer_flag = false;
    num_receive--;
    DEBUG('t', "%s: The thread %s received: %d\n",name, currentThread->GetName(), *msg);

    internal_lock->Release();
}
