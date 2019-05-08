#include "synch_console.hh"

SynchConsole::SynchConsole(const char *read_buffer, const char *write_buffer){

	console = new Console(read_buffer, write_buffer, SynchConsole::CheckCharAvail,
			              SynchConsole::WriteDone,this);
	can_read   = new Semaphore("read avail", 0);//Bloque al proceso hasta que alguien escriba
	write_done = new Semaphore("write done", 0);
	write      = new Lock("lock console write");
	read       = new Lock("lock console read");
}

SynchConsole::~SynchConsole(){
	delete can_read;
	delete write_done;
	delete write;
	delete read;
	delete console;
}

void
SynchConsole::PutChar(char ch){

	write->Acquire();
	console->PutChar(ch);
	write_done->P();
	write->Release();

}

char
SynchConsole::GetChar(){

	read->Acquire();
	can_read->P();
	char ch = console->GetChar();
	read->Release();

	return ch;
}

int
SynchConsole::PutString(char * buffer,int size){
	ASSERT(buffer!=NULL);
	write->Acquire();
	for(int i=0;i<size;i++){
		console->PutChar(buffer[i]);
		write_done->P();
	}
	write->Release();

	return size;
}

int
SynchConsole::GetString(char * buffer,int size){
	ASSERT(buffer!=NULL);
	read->Acquire();

	for(int i=0;i<size;i++){
		can_read->P();
		buffer[i] = console->GetChar();
	}

	read->Release();

	return size;
}

void
SynchConsole::CheckCharAvail(void* consol){
	ASSERT(consol!=NULL);
	((SynchConsole *)consol)->write_done->V();
}

void
SynchConsole::void WriteDone(void * consol){
	ASSERT(consol!=NULL);
	((SynchConsole *)consol)->can_read->V();
}
