/// Data structures to manage a UNIX-like directory of file names.
///
/// A directory is a table of pairs: *<file name, sector #>*, giving the name
/// of each file in the directory, and where to find its file header (the
/// data structure describing where to find the file's data blocks) on disk.
///
/// We assume mutual exclusion is provided by the caller.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_FILESYS_DIRECTORY__HH
#define NACHOS_FILESYS_DIRECTORY__HH


#include "open_file.hh"


/// For simplicity, we assume file names are <= 9 characters long.
const unsigned FILE_NAME_MAX_LEN = 9;

/// The following class defines a "directory entry", representing a file in
/// the directory.  Each entry gives the name of the file, and where the
/// file's header is to be found on disk.
///
/// Internal data structures kept public so that Directory operations can
/// access them directly.
class DirectoryEntry {
public:
    /// Is this directory entry in use?
    bool inUse;
    /// Location on disk to find the `FileHeader` for this file.
    unsigned sector;
    /// Text name for file, with +1 for the trailing `'\0'`.
    char name[FILE_NAME_MAX_LEN + 1];
};

/// The following class defines a UNIX-like “directory”.  Each entry in the
/// directory describes a file, and where to find it on disk.
///
/// The directory data structure can be stored in memory, or on disk.  When
/// it is on disk, it is stored as a regular Nachos file.
///
/// The constructor initializes a directory structure in memory; the
/// `FetchFrom`/`WriteBack` operations shuffle the directory information
/// from/to disk.
class Directory {
public:

    /// Initialize an empty directory with space for `size` files.
    Directory(int size);

    /// De-allocate the directory.
    ~Directory();

    /// Init directory contents from disk.
    void FetchFrom(OpenFile *file);

    /// Write modifications to directory contents back to disk.
    void WriteBack(OpenFile *file);

    /// Find the sector number of the `FileHeader` for file: `name`.
    int Find(const char *name);

    /// Add a file name into the directory.
    bool Add(const char *name, int newSector);

    /// Remove a file from the directory.
    bool Remove(const char *name);

    /// Print the names of all the files in the directory.
    void List() const;

    /// Verbose print of the contents of the directory -- all the file names
    /// and their contents.
    void Print() const;

private:
    int tableSize;  ///< Number of directory entries.
    DirectoryEntry *table;  ///< Table of pairs:
                            ///< *<file name, file header location>*.

    /// Find the index into the directory table corresponding to `name`.
    int FindIndex(const char *name);
};


#endif
