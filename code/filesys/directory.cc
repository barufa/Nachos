/// Routines to manage a directory of file names.
///
/// The directory is a table of fixed length entries; each entry represents a
/// single file, and contains the file name, and the location of the file
/// header on disk.  The fixed size of each directory entry means that we
/// have the restriction of a fixed maximum size for file names.
///
/// The constructor initializes an empty directory of a certain size; we use
/// ReadFrom/WriteBack to fetch the contents of the directory from disk, and
/// to write back any modifications back to disk.
///
/// Also, this implementation has the restriction that the size of the
/// directory cannot expand.  In other words, once all the entries in the
/// directory are used, no more files can be created.  Fixing this is one of
/// the parts to the assignment.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2018 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "directory.hh"
#include "directory_entry.hh"
#include "file_header.hh"
#include "lib/utility.hh"


/// Initialize a directory; initially, the directory is completely empty.  If
/// the disk is being formatted, an empty directory is all we need, but
/// otherwise, we need to call FetchFrom in order to initialize it from disk.
///
/// * `size` is the number of entries in the directory.
Directory::Directory(unsigned size)
{
    ASSERT(size > 0);
    raw.table     = new DirectoryEntry [size];
    raw.tableSize = size;
	raw.used      = 0;
    for (unsigned i = 0; i < size; i++){
		raw.table[i].inUse = false;
		raw.table[i].isDir = false;
		raw.table[i].sector = NOT_ASSIGNED;
	}
}

/// De-allocate directory data structure.
Directory::~Directory()
{
    delete [] raw.table;
	raw.tableSize = 0;
	raw.used      = 0;
}

/// Read the contents of the directory from disk.
///
/// * `file` is file containing the directory contents.
void
Directory::FetchFrom(OpenFile * file)
{
    ASSERT(file != nullptr);
    file->ReadAt((char *) raw.table,
      raw.tableSize * sizeof(DirectoryEntry), 0);
}

/// Write any modifications to the directory back to disk.
///
/// * `file` is a file to contain the new directory contents.
void
Directory::WriteBack(OpenFile * file)
{
    ASSERT(file != nullptr);
    file->WriteAt((char *) raw.table, raw.tableSize * sizeof(DirectoryEntry), 0);
}

/// Look up file name in directory, and return its location in the table of
/// directory entries.  Return -1 if the name is not in the directory.
///
/// * `name` is the file name to look up.
int
Directory::FindIndex(const char * name, bool isDir)
{
    ASSERT(name != nullptr);

    for (unsigned i = 0; i < raw.tableSize; i++)
        if (raw.table[i].inUse &&
			raw.table[i].isDir == isDir &&
          !strncmp(raw.table[i].name, name, FILE_NAME_MAX_LEN))
            return i;

    return -1; // name not in directory
}

/// Look up file name in directory, and return the disk sector number where
/// the file's header is stored.  Return -1 if the name is not in the
/// directory.
///
/// * `name` is the file name to look up.
int
Directory::Find(const char * name, bool isDir)
{
    ASSERT(name != nullptr);

    int i = FindIndex(name,isDir);
    if (i != -1)
        return raw.table[i].sector;

    return -1;
}

/// Add a file into the directory.  Return true if successful; return false
/// if the file name is already in the directory, or if the directory is
/// completely full, and has no more space for additional file names.
///
/// * `name` is the name of the file being added.
/// * `newSector` is the disk sector containing the added file's header.
bool
Directory::Add(const char * name, int newSector, bool isDir)
{
    ASSERT(name != nullptr);

	printf("Agregando %s como %s\n",name,isDir? "Directorio":"Archivo");

    if (FindIndex(name, true) != -1 || FindIndex(name, false)!=-1){
		return false;
	}

	if(raw.used==raw.tableSize){
		//No space, add a new entry
		DirectoryEntry * newTable = new DirectoryEntry [raw.tableSize+10];
		for (unsigned i = 0; i < raw.tableSize + 10; i++){
			if(i < raw.tableSize){
				newTable[i] = raw.table[i];
			}else{
				newTable[i].inUse = false;
				newTable[i].isDir = false;
				newTable[i].sector = NOT_ASSIGNED;
			}
		}
		delete raw.table;
		raw.table = newTable;
	}

    for (unsigned i = 0; i < raw.tableSize; i++){
		if (!raw.table[i].inUse) {
            raw.table[i].inUse = true;
			raw.table[i].isDir = isDir;
            strncpy(raw.table[i].name, name, FILE_NAME_MAX_LEN);
            raw.table[i].sector = newSector;
			raw.used++;
            return true;
        }
	}

    return false; // no space.  Fix when we have extensible files.
}

/// Remove a file name from the directory.   Return true if successful;
/// return false if the file is not in the directory.
///
/// * `name` is the file name to be removed.
bool
Directory::Remove(const char * name)
{
    ASSERT(name != nullptr);

    int i = FindIndex(name,false);
    if (i == -1){
		i = FindIndex(name,true);
	}

	if (i == -1){
		return false;  // name not in directory
	}
	raw.used--;
    raw.table[i].inUse = false;
	raw.table[i].isDir = false;
	raw.table[i].sector = NOT_ASSIGNED;
    return true;
}

/// List all the file names in the directory.
void
Directory::Get_List(const char * ind) const
{

	unsigned l = strlen(ind);
	char * p = new char[l+2];
	strncpy(p,ind,l);
	p[l] = '\t';
	p[l+1] = '\0';
	DEBUG('F',"%sSize: %u\n",ind,raw.tableSize);
    for (unsigned i = 0; i < raw.tableSize; i++){
		DEBUG('F',"%sraw.table[%u].inUse = %s\n",ind,i,raw.table[i].inUse? "True":"False");
		DEBUG('F',"%sraw.table[%u].isDir = %s\n",ind,i,raw.table[i].isDir? "True":"False");
		if (raw.table[i].inUse){
			printf("%s%s: %s\n",ind, raw.table[i].isDir? "Dir":"File", raw.table[i].name);
			if(raw.table[i].isDir){
				Directory * dir = new Directory;
				OpenFile * dir_file = new OpenFile(raw.table[i].sector);
				dir->FetchFrom(dir_file);
				dir->Get_List(p);
				delete dir_file;
				delete dir;
			}
		}
	}
}

/// List all the file names in the directory, their `FileHeader` locations,
/// and the contents of each file.  For debugging.
void
Directory::Print() const
{
    FileHeader * hdr = new FileHeader;
	Directory * dir  = new Directory(NUM_DIRECT);
	printf("************************************\n");
    printf("Directory contents:\n");
    for (unsigned i = 0; i < raw.tableSize; i++){
		if (raw.table[i].inUse && !raw.table[i].isDir) {
			printf("\nFile entry.\n"
			"    Name: %s\n"
			"    Sector: %u\n",
			raw.table[i].name, raw.table[i].sector);
			hdr->FetchFrom(raw.table[i].sector);
			hdr->Print();
		}
		if (raw.table[i].inUse && raw.table[i].isDir){
			printf("\nDir entry.\n"
			"    Name: %s\n"
			"    Sector: %u\n",
			raw.table[i].name, raw.table[i].sector);
			OpenFile * dir_file = new OpenFile(raw.table[i].sector);
			dir->FetchFrom(dir_file);
			dir->Print();
			delete dir_file;
		}
	}
    printf("************************************\n");
    delete hdr;
	delete dir;
}

const RawDirectory *
Directory::GetRaw() const
{
    return &raw;
}

bool
Directory::Empty() const
{
	for (unsigned i = 0; i < raw.tableSize; i++){
		if (raw.table[i].inUse) {
			return false;
		}
    }
	return true;
}

void
Directory::Clean(Bitmap * freeMap){

    for (unsigned i = 0; i < raw.tableSize; i++){
        if(raw.table[i].inUse){
            if(raw.table[i].isDir){
                Directory * dir = new Directory(NUM_DIR_ENTRIES);
                OpenFile * dir_file = new OpenFile(raw.table[i].sector);
                dir->FetchFrom(dir_file);
                dir->Clean(freeMap);
				if(freeMap->Test(raw.table[i].sector)){
					freeMap->Clear(raw.table[i].sector);
				}
                delete dir;
                delete dir_file;
            }
            raw.table[i].inUse = false;
			raw.table[i].isDir = false;
			raw.table[i].sector = NOT_ASSIGNED;
        }
    }
}
