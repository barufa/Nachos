#include "syscall.h"

#define THREAD_NUM 5
#define PUT(s) (Write((s),strlen((s)),CONSOLE_OUTPUT))

static inline unsigned
strlen(const char * s)
{
    unsigned i;

    for (i = 0; s[i] != '\0'; i++);
    return i;
}

int
main(int argc,char * argv[])
{
	OpenFileId id;
	Create("File.txt");

	int i = 0,j = 0;
    char * arg[] = {"userland/TestWriteAux", NULL};
	SpaceId ids[THREAD_NUM];

	for(i = 0;i<THREAD_NUM;i++){
		ids[i] = Exec(arg[0],arg,1);
	}

	id = Open("File.txt");
	PUT("Start Writer 1\n");
	for (i = 0;i<30;i++){
		Write("Test Writer 1 \n",strlen("Test Writer 1 \n"),id);
        for (j = 0; j<5000;j++);
	}
	Close(id);
    PUT("Finish Writer 1\n");
	for(i = 0;i<THREAD_NUM;i++){
		Join(ids[i]);
	}
	Halt();
}
