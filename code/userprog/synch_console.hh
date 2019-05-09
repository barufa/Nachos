#ifndef NACHOS_SYNCHCONSOLE__HH
#define NACHOS_SYNCHCONSOLE__HH

#include "machine/console.hh"
#include "synch.hh"

class SynchConsole{
  public:

    SynchConsole(const char * s,const char * read_buffer, const char *write_buffer);
  	~SynchConsole();
  	void PutChar(char ch);
  	char GetChar();

  	int PutString(char * buffer,int size);
  	int GetString(char * buffer,int size);

  private:
    static void CheckCharAvail(void*);
    static void WriteDone(void*);
    Semaphore *can_read;
    Semaphore *write_done;
    Console *console;
    Lock *read;
    Lock *write;
    const char * name;
};

#endif
