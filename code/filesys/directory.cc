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
#include "threads/system.hh"

unsigned
Directory::Extend_Table(unsigned cnt)
{
    unsigned old_sz = raw.tableSize;

    raw.tableSize += cnt;
    DirectoryEntry * newTable = new DirectoryEntry [raw.tableSize];
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (i < old_sz) {
            newTable[i].inUse  = raw.table[i].inUse;
            newTable[i].isDir  = raw.table[i].isDir;
            newTable[i].sector = raw.table[i].sector;
            strncpy(newTable[i].name, raw.table[i].name, FILE_NAME_MAX_LEN);
        } else {
            newTable[i].inUse  = false;
            newTable[i].isDir  = false;
            newTable[i].sector = NOT_ASSIGNED;
        }
    }
    delete raw.table;
    raw.table = newTable;
    return old_sz;
}

void
Directory::Get_Lock()
{
    if (sectornumber != NOT_ASSIGNED) {
        ASSERT(filetable->find(sectornumber) != nullptr);
        Filenode * node = filetable->find(sectornumber);
        node->Dir_Lock->Acquire();
        DEBUG('D', "Tomando Dir Lock:%x Thread:%s\n", node->Dir_Lock,
          currentThread->GetName());
        OpenFile * Dir_file = new OpenFile(sectornumber);
        FetchFrom(Dir_file);
    }
}

void
Directory::Release_Lock()
{
    if (sectornumber != NOT_ASSIGNED) {
        ASSERT(filetable->find(sectornumber) != nullptr);
        Filenode * node = filetable->find(sectornumber);
        DEBUG('D', "Liberando Dir Lock:%x Thread:%s\n", node->Dir_Lock,
          currentThread->GetName());
        node->Dir_Lock->Release();
    }
}

/// Initialize a directory; initially, the directory is completely empty.  If
/// the disk is being formatted, an empty directory is all we need, but
/// otherwise, we need to call FetchFrom in order to initialize it from disk.
///
/// * `size` is the number of entries in the directory.
Directory::Directory(unsigned size)
{
    ASSERT(size > 0);
    sectornumber  = NOT_ASSIGNED;
    raw.table     = new DirectoryEntry [size];
    raw.tableSize = size;
    raw.used      = 0;
    for (unsigned i = 0; i < size; i++) {
        raw.table[i].inUse  = false;
        raw.table[i].isDir  = false;
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
    file->ReadAt((char *) &raw.tableSize, sizeof(raw.tableSize), 0);
    raw.table = new DirectoryEntry[raw.tableSize];
    file->ReadAt((char *) raw.table, raw.tableSize * sizeof(DirectoryEntry), 8);
    sectornumber = file->Get_Sector();
    if (filetable->find(sectornumber) == nullptr) {
        filetable->add_file("Dir", sectornumber);
    }
}

/// Write any modifications to the directory back to disk.
///
/// * `file` is a file to contain the new directory contents.
void
Directory::WriteBack(OpenFile * file)
{
    ASSERT(file != nullptr);
    file->WriteAt((const char *) &raw.tableSize, sizeof(raw.tableSize), 0);
    file->WriteAt((const char *) raw.table,
      raw.tableSize * sizeof(DirectoryEntry), 8);
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

    int i = FindIndex(name, isDir);
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
    Get_Lock();

    if (FindIndex(name, true) != -1 || FindIndex(name, false) != -1) {
        // The name is already in use
        Release_Lock();
        return false;
    }
    // Look for an empty entry
    unsigned idx = NOT_ASSIGNED;
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (!raw.table[i].inUse) {
            idx = i;
            break;
        }
    }
    // Create a new entry
    if (idx == NOT_ASSIGNED) {
        idx = Extend_Table(2);
    }
    // Adding File
    raw.table[idx].inUse = true;
    raw.table[idx].isDir = isDir;
    strncpy(raw.table[idx].name, name, FILE_NAME_MAX_LEN);
    raw.table[idx].sector = newSector;
    Release_Lock();
    return true;
}

/// Remove a file name from the directory.   Return true if successful;
/// return false if the file is not in the directory.
///
/// * `name` is the file name to be removed.
unsigned
Directory::Remove(const char * name)
{
    ASSERT(name != nullptr);

    int i = FindIndex(name, false);
    if (i == -1) {
        i = FindIndex(name, true);
    }

    if (i == -1) {
        return 0; // name not in directory
    }

    Get_Lock();
    unsigned sec = raw.table[i].sector;
    raw.table[i].inUse  = false;
    raw.table[i].isDir  = false;
    raw.table[i].sector = NOT_ASSIGNED;
    DEBUG('D', "Delete Index %d - Sector %d\n", i, sec);
    Release_Lock();
    return (sec == NOT_ASSIGNED ? 0 : sec);
}

/// List all the file names in the directory.
void
Directory::Get_List(const char * ind) const
{
    unsigned l = strlen(ind);
    char * p   = new char[l + 2];

    strncpy(p, ind, l);
    p[l]     = '\t';
    p[l + 1] = '\0';
    DEBUG('D', "%sSize: %u\n", ind, raw.tableSize);
    for (unsigned i = 0; i < raw.tableSize; i++) {
        DEBUG('D', "%sraw.table[%u].inUse = %s\n", ind, i,
          raw.table[i].inUse ? "True" : "False");
        DEBUG('D', "%sraw.table[%u].isDir = %s\n", ind, i,
          raw.table[i].isDir ? "True" : "False");
        if (raw.table[i].inUse) {
            printf("%s%s: %s\n", ind, raw.table[i].isDir ? "Dir" : "File",
              raw.table[i].name);
            if (raw.table[i].isDir) {
                Directory * dir     = new Directory;
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
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse && !raw.table[i].isDir) {
            printf("\nFile entry.\n"
              "    Name: %s\n"
              "    Sector: %u\n",
              raw.table[i].name, raw.table[i].sector);
            hdr->FetchFrom(raw.table[i].sector);
            hdr->Print();
        }
        if (raw.table[i].inUse && raw.table[i].isDir) {
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
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse) {
            return false;
        }
    }
    return true;
}

void
Directory::Clean(Bitmap * freeMap)
{
    Get_Lock();
    // Borro el contenido
    for (unsigned i = 0; i < raw.tableSize; i++) {
        if (raw.table[i].inUse && freeMap->Test(raw.table[i].sector)) {
            if (raw.table[i].isDir) {
                Directory * dir     = new Directory(NUM_DIR_ENTRIES);
                OpenFile * dir_file = new OpenFile(raw.table[i].sector);
                dir->FetchFrom(dir_file);
                dir->Clean(freeMap);
                FileHeader * header = new FileHeader;
                header->FetchFrom(raw.table[i].sector);
                header->Deallocate(freeMap);
                freeMap->Clear(raw.table[i].sector);
                delete dir;
                delete dir_file;
                delete header;
                DEBUG('D', "Liberando Carpeta %u\n", raw.table[i].sector);
            } else {
                DEBUG('D', "Liberando Archivo %u\n", raw.table[i].sector);
                Filenode * node = filetable->find(raw.table[i].sector);
                if (node != nullptr && node->users != 0) {
                    node->remove = true;
                } else {
                    FileHeader * header = new FileHeader;
                    header->FetchFrom(raw.table[i].sector);
                    header->Deallocate(freeMap);
                    freeMap->Clear(raw.table[i].sector);
                }
            }
            raw.table[i].inUse  = false;
            raw.table[i].isDir  = false;
            raw.table[i].sector = NOT_ASSIGNED;
        }
    }
    Release_Lock();
} // Directory::Clean
