#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"


bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        while(!machine->ReadMem(userAddress++, 1, &temp));
        *outString = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

/// Copy a byte array from virtual machine to host.
void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);
	for(unsigned count = 0;count<byteCount;count++)
    {
        int tmp;
		while(!machine->ReadMem(userAddress++, 1, &tmp));
		outBuffer[count] = (unsigned char) tmp;
	}
	return;
}

/// Copy a byte array from host to virtual machine.
void WriteBufferToUser(int userAddress, const char *buffer, unsigned byteCount)
{
	ASSERT(userAddress != 0);
	ASSERT(buffer != nullptr);
	ASSERT(byteCount != 0);

	for(unsigned i = 0;i<byteCount;i++)
	{
    while(!machine->WriteMem(userAddress++, 1, (int)buffer[i]));
	}

	return;
}

/// Copy a C string from host to virtual machine.
void WriteStringToUser(const char *string, int userAddress)
{
	ASSERT(userAddress != 0);
	ASSERT(string != nullptr);

	for(unsigned i = 0;string[i] != '\0';i++)
	{
		while(!machine->WriteMem(userAddress++, 1, string[i]));
	}

	return;
}
