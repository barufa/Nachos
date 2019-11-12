/// Routines to synchronously access the disk.  The physical disk is an
/// asynchronous device (disk requests return immediately, and an interrupt
/// happens later on).  This is a layer on top of the disk providing a
/// synchronous interface (requests wait until the request completes).
///
/// Use a semaphore to synchronize the interrupt handlers with the pending
/// requests.  And, because the physical disk can only handle one operation
/// at a time, use a lock to enforce mutual exclusion.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "synch_disk.hh"

static const char * sector_empty[SECTOR_SIZE];

/// Initialize the synchronous interface to the physical disk, in turn
/// initializing the physical disk.
///
/// * `name` is a UNIX file name to be used as storage for the disk data
///   (usually, `DISK`).
SynchDisk::SynchDisk(const char * name)
{
    lock = new Lock("synch disk lock");
    disk = new CacheDisk(name);
}

/// De-allocate data structures needed for the synchronous disk abstraction.
SynchDisk::~SynchDisk()
{
    delete disk;
    delete lock;
}

/// Read the contents of a disk sector into a buffer.  Return only after the
/// data has been read.
///
/// * `sectorNumber` is the disk sector to read.
/// * `data` is the buffer to hold the contents of the disk sector.
void
SynchDisk::ReadSector(int sectorNumber, char * data)
{
    ASSERT(data != nullptr);
    lock->Acquire(); // Only one disk I/O at a time.
    disk->ReadSector(sectorNumber, data);
    lock->Release();
}

/// Write the contents of a buffer into a disk sector.  Return only
/// after the data has been written.
///
/// * `sectorNumber` is the disk sector to be written.
/// * `data` are the new contents of the disk sector.
void
SynchDisk::WriteSector(int sectorNumber, const char * data)
{
    ASSERT(data != nullptr);

    lock->Acquire(); // only one disk I/O at a time
    disk->WriteSector(sectorNumber, data);
    lock->Release();
}

void
SynchDisk::ClearSector(int sectorNumber)
{
    WriteSector(sectorNumber, (const char *) sector_empty);
}
