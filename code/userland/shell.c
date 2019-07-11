#include "syscall.h"


#define MAX_LINE_SIZE  60
#define MAX_ARG_COUNT  12
#define ARG_SEPARATOR  ' '

#define NULL  ((void *) 0)

static inline unsigned
strlen(const char *s)
{

	if(!s)return 0;

    unsigned i;
    for (i = 0; s[i] != '\0'; i++);
    return i;
}

static inline void
WritePrompt(OpenFileId output)
{
    static const char PROMPT[] = "--> ";
    Write(PROMPT, sizeof PROMPT - 1, output);
}

static inline void
WriteError(const char *description, OpenFileId output)
{
    static const char PREFIX[] = "Error: ";
    static const char SUFFIX[] = "\n";

    Write(PREFIX, sizeof PREFIX - 1, output);
    if(description)Write(description, strlen(description), output);
    Write(SUFFIX, sizeof SUFFIX - 1, output);
}

static unsigned
ReadLine(char *buffer, unsigned size, OpenFileId input)
{
	if(!buffer)return 0;
		
	unsigned i = 0;
	size--;
	
    for (i = 0; i < size; i++) {
        buffer[i] = '1';
        if(Read(&buffer[i], 1, input)!=1){
			Write("Error en read\n",strlen("Error en read\n"),CONSOLE_OUTPUT);
			Halt();
		}
        if (buffer[i] == '\n' || buffer[i] == '\0') {
            break;
        }
    }
    buffer[i] = '\0';
    
    return i;
}

static int
PrepareArguments(char *line, char **argv, unsigned argvSize)
{
    // TO DO: how to make sure that `line` and `argv` are not `NULL`?, and
    //        for `argvSize`, what precondition should be fulfilled?
    //
    // PENDIENTE: use `bool` instead of `int` as return type; for doing this,
    //            given that we are in C and not C++, it is convenient to
    //            include `stdbool.h`.

    unsigned argCount;

    argv[0] = line;
    argCount = 1;

    // Traverse the whole line and replace spaces between arguments by null
    // characters, so as to be able to treat each argument as a standalone
    // string.
    //
    // TO DO: what happens if there are two consecutive spaces?, and what
    //        about spaces at the beginning of the line?, and at the end?
    //
    // TO DO: what if the user wants to include a space as part of an
    //        argument?
	for (unsigned i = 0; i<MAX_LINE_SIZE && line[i] != '\0'; i++){
		if (line[i] == ARG_SEPARATOR) {
			if (argCount == argvSize - 1)return 0;
			line[i]='\0';
			do i++; while(i<MAX_LINE_SIZE && line[i]==' ');
			if(i<MAX_LINE_SIZE && line[i]!='\0'){
				argv[argCount++] = &line[i];
			}
		}
	}

    argv[argCount] = NULL;
    return 1;
}

static void
add_userland(char * line){
	
	char buffer[MAX_LINE_SIZE] = "userland/";
	int j = strlen("userland/");
		
	for(int i=0;i<strlen(line);i++){
		buffer[j++] = line[i];
	}
	
	for(int i=0;i<strlen(buffer);i++){
		line[i]=buffer[i];
	}
	
	return;
}

static int 
isexit(const char * line){
	if(!line)return 0;
	int i=0;
	if(line[0]=='&')i++;
	int ret = (line[i+0]=='e') &&
			  (line[i+1]=='x') &&
			  (line[i+2]=='i') &&
			  (line[i+3]=='t');
	return ret;
}

int
main(void)
{
    const OpenFileId INPUT  = CONSOLE_INPUT;
    const OpenFileId OUTPUT = CONSOLE_OUTPUT;
    char   line[MAX_LINE_SIZE];
    char * argv[MAX_ARG_COUNT];
    int    background;

    for (;;) {

        background=0;

        WritePrompt(OUTPUT);

        const unsigned lineSize = ReadLine(line, MAX_LINE_SIZE, INPUT);
        
		if(lineSize == 0 || isexit(line)){
			Halt();
		}
        
        if(line[0]=='&'){
            background = 1;
			for(int i=0; i<lineSize;i++){
                line[i] = line[i+1];
            }
		}

        if(PrepareArguments(line, argv, MAX_ARG_COUNT) == 0) {
            WriteError("too many arguments.", OUTPUT);
            continue;
        }

        const SpaceId newProc = Exec(line, argv, background ? 0 : 1 );
        if(!background) Join(newProc);


        // TO DO: check for errors when calling `Exec`; this depends on how
        //        errors are reported.
        // TO DO: is it necessary to check for errors after `Join` too, or
        //        can you be sure that, with the implementation of the system
        //        call handler you made, it will never give an error?; what
        //        happens if tomorrow the implementation changes and new
        //        error conditions appear?
    }

    return 0;  // Never reached.
}
