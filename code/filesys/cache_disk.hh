/// Data structures to export a cache interface to the raw disk device.

#ifndef NACHOS_FILESYS_CACHEDISK__HH
#define NACHOS_FILESYS_SYNCHDISK__HH

#include "machine/disk.hh"
#include "threads/synch.hh"
#include "lib/list.hh"

const unsigned CACHE_SIZE = 64;

typedef struct{
    char * data;
    bool modified;
    unsigned sector;
}SectorCache;

class CacheDisk {
public:

    /// Initialize a synchronous disk, by initializing the raw Disk.
    CacheDisk(const char * name);

    /// De-allocate the synch disk data.
    ~CacheDisk();

    /// Read/write a disk sector, returning only once the data is actually
    /// read or written.  These call `Disk::ReadRequest`/`WriteRequest` and
    /// then wait until the request is done.

    void
    ReadSector(int sectorNumber, char * data);

    void
    WriteSector(int sectorNumber, const char * data);

    /// Called by the disk device interrupt handler, to signal that the
    /// current disk operation is complete.
    void
    RequestDone();

private:
    Disk * disk;           ///< Raw disk device.
    List<SectorCache> * cache;
    Semaphore * semaphore;

    SectorCache *
    CacheAdd(int sectorNumber);

    void
    CacheRemove();

    bool
    IsOnCache(int sectorNumber,SectorCache ** Sec);

    void
    InternalWrite(int sectorNumber, const char * data);

    void
    InternalRead(int sectorNumber, char * data);
};


#endif /* ifndef NACHOS_FILESYS_SYNCHDISK__HH */
