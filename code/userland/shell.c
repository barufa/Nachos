#include "syscall.h"

#include <stdbool.h>
#include <assert.h>

#define MAX_ARG_COUNT 9
#define MAX_LINE_SIZE 50
#define ARG_SEPARATOR ' '

static inline unsigned
strlen(const char * s)
{
        if (!s) return 0;

        unsigned i;
        for (i = 0; s[i] != '\0'; i++);
        return i;
}

static void
reverse(char s[])
{
        int i, j;
        char c;

        for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
                c = s[i];
                s[i] = s[j];
                s[j] = c;
        }
}

static void
itoa(int n, char s[])
{
        int i, sign = n;

        if (sign < 0){
                n = -n;
	}
        i = 0;
        do {
                s[i++] = n % 10 + '0';
        } while ((n /= 10) > 0);
        if (sign < 0){
            s[i++] = '-';
	}
        s[i] = '\0';
        reverse(s);
}

void PutStr(const char * str){
    Write(str, strlen(str), CONSOLE_OUTPUT);
    Write("\n", strlen("\n"), CONSOLE_OUTPUT);
}

static inline void
WritePrompt(OpenFileId output)
{
        static const char PROMPT[] = "--> ";

        Write(PROMPT, sizeof PROMPT - 1, output);
}

static inline void
WriteError(const char * description, OpenFileId output)
{
        static const char PREFIX[] = "Error: ";
        static const char SUFFIX[] = "\n";

        Write(PREFIX, sizeof PREFIX - 1, output);
        if (description) Write(description, strlen(description), output);
        Write(SUFFIX, sizeof SUFFIX - 1, output);
}

static unsigned
ReadLine(char * buffer, unsigned size, OpenFileId input)
{
        if (!buffer) return 0;

        unsigned i = 0;
        size--;

        for (i = 0; i < size; i++) {
                if (Read(&buffer[i], 1, input) != 1) {
			WriteError("Problem with syscall Read",CONSOLE_OUTPUT);
                        return 0;
                }
                if (buffer[i] == '\n' || buffer[i] == '\0') {
                        break;
                }
        }
        buffer[i] = '\0';

        return i;
}

static bool
PrepareArguments(char * line, char ** argv, unsigned argvSize)
{
	assert(argvSize>=1);
	assert(line!=NULL);
	assert(argv!=NULL);

        unsigned i, argCount = 0, start = 0, size = strlen(line);
	line[strlen(line)] = ARG_SEPARATOR;
	line[strlen(line)+1] = '\0';

        // Traverse the whole line and replace spaces between arguments by null
        // characters, so as to be able to treat each argument as a standalone
        // string.

	//Si arranca con varios espacios lo salteo
	for(start = 0;start < MAX_LINE_SIZE && line[start]==ARG_SEPARATOR && line[start]!='\0';start++);

	//Se que line[start]!=ARG_SEPARATOR
        for (i = start; i < MAX_LINE_SIZE && line[start] != '\0' && line[i] != '\0'; i++) {
                if (line[i] == ARG_SEPARATOR) {
                        if (argCount == argvSize){
				return false;
			}
			//De start a i tengo un argumento
			argv[argCount++] = &line[start];
			line[i++] = '\0';
			//Muevo el i y start
			while(i < MAX_LINE_SIZE && line[i] != '\0' && line[i]==ARG_SEPARATOR)i++;
			start = i;
                }
        }
        argv[argCount] = NULL;

	// for(int i=0;argv[i]!=NULL;i++){
	// 	PutStr(argv[i]);
	// }

        return true;
} /* PrepareArguments */

static int
isexit(const char * line)
{
        if (!line) return 0;

        int i = 0;
        if (line[0] == '&') i++;
        int ret = (line[i + 0] == 'e') &&
                  (line[i + 1] == 'x') &&
                  (line[i + 2] == 'i') &&
                  (line[i + 3] == 't');
        return ret;
}

static int
isret(const char * line,int last_ret)
{
        if (!line) return 0;

        int i = 0;
        if (line[0] == '&') i++;
        int ret = (line[i + 0] == '$') &&
                  (line[i + 1] == '?');

        if(ret) {
                char snum[20];
                itoa(last_ret, snum);
                Write(snum, strlen(snum), CONSOLE_OUTPUT);
                Write("\n", strlen("\n"), CONSOLE_OUTPUT);
        }

        return ret;
}

int
main(void)
{
        const OpenFileId INPUT  = CONSOLE_INPUT;
        const OpenFileId OUTPUT = CONSOLE_OUTPUT;
        char root_line[MAX_LINE_SIZE+1] = {'u','s','e','r','l','a','n','d','/'};
	char * line = &root_line[9];
        char * argv[MAX_ARG_COUNT+1];
        int background, last_ret = 0;

        for (;;) {
                background = 0;

                WritePrompt(OUTPUT);

                const unsigned lineSize = ReadLine(line, MAX_LINE_SIZE, INPUT);

                if (lineSize == 0) {
                        continue;
                }
                if(isexit(line)) {
                        Halt();
                }else if(isret(line,last_ret)) {
                        continue;
                }

                if (line[0] == '&') {
                        background = 1;
                        for (int i = 0; i < lineSize; i++) {
                                line[i] = line[i + 1];
                        }
                }

                if (!PrepareArguments(root_line, argv, MAX_ARG_COUNT)) {
                        WriteError("too many arguments.", OUTPUT);
                        continue;
                }

                const SpaceId newProc = Exec(root_line, argv, background ? 0 : 1);
                if (!background) {
                        last_ret = Join(newProc);
                }

        }

        Exit(0); // Never reached.
} /* main */
