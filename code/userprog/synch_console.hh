#ifndef NACHOS_SYNCHCONSOLE__HH
#define NACHOS_SYNCHCONSOLE__HH

#include "machine/console.hh"
#include "synch.hh"

class SynchConsole {
public:

    SynchConsole(const char * name, const char * read_buffer = NULL,
      const char * write_buffer = NULL);
    ~SynchConsole();
    void
    PutChar(char ch);
    char
    GetChar();

    int
    PutString(char * buffer, int size);
    int
    GetString(char * buffer, int size);

private:
    static void
    CheckCharAvail(void *);
    static void
    WriteDone(void *);
    Semaphore * can_read;
    Semaphore * write_done;
    Console * console;
    Lock * read;
    Lock * write;
    const char * name;
};

#endif /* ifndef NACHOS_SYNCHCONSOLE__HH */
