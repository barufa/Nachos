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


#include "file_header.hh"
#include "directory.hh"
#include "lib/utility.hh"


/// Initialize a directory; initially, the directory is completely empty.  If
/// the disk is being formatted, an empty directory is all we need, but
/// otherwise, we need to call FetchFrom in order to initialize it from disk.
///
/// * `size` is the number of entries in the directory.
Directory::Directory(int size)
{
    ASSERT(size > 0);
    table = new DirectoryEntry [size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
        table[i].inUse = false;
}

/// De-allocate directory data structure.
Directory::~Directory()
{
    delete [] table;
}

/// Read the contents of the directory from disk.
///
/// * `file` is file containing the directory contents.
void
Directory::FetchFrom(OpenFile *file)
{
    ASSERT(file != nullptr);
    file->ReadAt((char *) table, tableSize * sizeof (DirectoryEntry), 0);
}

/// Write any modifications to the directory back to disk.
///
/// * `file` is a file to contain the new directory contents.
void
Directory::WriteBack(OpenFile *file)
{
    ASSERT(file != nullptr);
    file->WriteAt((char *) table, tableSize * sizeof (DirectoryEntry), 0);
}

/// Look up file name in directory, and return its location in the table of
/// directory entries.  Return -1 if the name is not in the directory.
///
/// * `name` is the file name to look up.
int
Directory::FindIndex(const char *name)
{
    ASSERT(name != nullptr);

    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse
              && !strncmp(table[i].name, name, FILE_NAME_MAX_LEN))
            return i;
    return -1;  // name not in directory
}

/// Look up file name in directory, and return the disk sector number where
/// the file's header is stored.  Return -1 if the name is not in the
/// directory.
///
/// * `name` is the file name to look up.
int
Directory::Find(const char *name)
{
    ASSERT(name != nullptr);

    int i = FindIndex(name);
    if (i != -1)
        return table[i].sector;
    return -1;
}

/// Add a file into the directory.  Return true if successful; return false
/// if the file name is already in the directory, or if the directory is
/// completely full, and has no more space for additional file names.
///
/// * `name` is the name of the file being added.
/// * `newSector` is the disk sector containing the added file's header.
bool
Directory::Add(const char *name, int newSector)
{
    ASSERT(name != nullptr);

    if (FindIndex(name) != -1)
        return false;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = true;
            strncpy(table[i].name, name, FILE_NAME_MAX_LEN);
            table[i].sector = newSector;
            return true;
        }
    return false;  // no space.  Fix when we have extensible files.
}

/// Remove a file name from the directory.   Return true if successful;
/// return false if the file is not in the directory.
///
/// * `name` is the file name to be removed.
bool
Directory::Remove(const char *name)
{
    ASSERT(name != nullptr);

    int i = FindIndex(name);
    if (i == -1)
        return false;  // name not in directory
    table[i].inUse = false;
    return true;
}

/// List all the file names in the directory.
void
Directory::List() const
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
            printf("%s\n", table[i].name);
}

/// List all the file names in the directory, their `FileHeader` locations,
/// and the contents of each file.  For debugging.
void
Directory::Print() const
{
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse) {
            printf("\nDirectory entry.\n"
                   "    Name: %s\n"
                   "    Sector: %u\n",
                   table[i].name, table[i].sector);
            hdr->FetchFrom(table[i].sector);
            hdr->Print();
        }
    printf("\n");
    delete hdr;
}
