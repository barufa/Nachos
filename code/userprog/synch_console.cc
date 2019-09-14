#include "synch_console.hh"

SynchConsole::SynchConsole(const char * s, const char * read_buffer,
  const char * write_buffer)
{
    DEBUG('a', "Creando %s consola syncronizada\n", s);
    console = new Console(read_buffer, write_buffer,
        SynchConsole::CheckCharAvail,
        SynchConsole::WriteDone, this);
    can_read   = new Semaphore("read avail", 0);// Bloque al proceso hasta que alguien escriba
    write_done = new Semaphore("write done", 0);
    write      = new Lock("lock console write");
    read       = new Lock("lock console read");
    name       = s;
}

SynchConsole::~SynchConsole()
{
    DEBUG('a', "Borrando consola %s syncronizada\n", name);
    delete can_read;
    delete write_done;
    delete write;
    delete read;
    delete console;
}

void
SynchConsole::PutChar(char ch)
{
    DEBUG('a', "%s: Llamando a putchar\n", name);
    write->Acquire();
    DEBUG('a', "%s: Escribiendo...\n", name);
    console->PutChar(ch);
    write_done->P();
    write->Release();
}

char
SynchConsole::GetChar()
{
    DEBUG('a', "%s: Llamando a getchar\n", name);
    read->Acquire();
    can_read->P();
    DEBUG('a', "%s: Leyendo...\n", name);
    char ch = console->GetChar();
    read->Release();

    return ch;
}

int
SynchConsole::PutString(char * buffer, int size)
{
    ASSERT(buffer != NULL);
    write->Acquire();
    for (int i = 0; i < size; i++) {
        console->PutChar(buffer[i]);
        write_done->P();
    }
    write->Release();

    return size;
}

int
SynchConsole::GetString(char * buffer, int size)
{
    ASSERT(buffer != NULL);
    read->Acquire();
    int i;
    for (i = 0; i < size; i++) {
        can_read->P();
        buffer[i] = console->GetChar();
        if (buffer[i] == '\0')
            break;
    }
    buffer[i] = '\0';
    read->Release();

    return i;
}

void
SynchConsole::CheckCharAvail(void * consol)
{
    ASSERT(consol != NULL);
    ((SynchConsole *) consol)->can_read->V();
}

void
SynchConsole::WriteDone(void * consol)
{
    ASSERT(consol != NULL);
    ((SynchConsole *) consol)->write_done->V();
}
