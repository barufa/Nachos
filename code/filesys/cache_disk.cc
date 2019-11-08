#include "cache_disk.hh"
#include <stdlib.h>
#include <unistd.h>

/////////////////////////////////Internal Staff/////////////////////////////////

static unsigned sector_num = 0;
static unsigned size = 0;

static bool
aux_cmp_cache(SectorCache sec)
{
    return sec.sector == sector_num;
}

static void
aux_get_sz(SectorCache sec)
{
    size++;
}

bool
operator == (const SectorCache& x, const SectorCache& y)
{
    return x.sector == y.sector && x.modified == y.modified;
}

/// Disk interrupt handler.  Need this to be a C routine, because C++ cannot
/// handle pointers to member functions.
static void
DiskRequestDone(void * arg)
{
    ASSERT(arg != nullptr);
    CacheDisk * disk = (CacheDisk *) arg;
    disk->RequestDone();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CacheDisk::CacheDisk(const char * name)
{
    disk      = new Disk(name, DiskRequestDone, this);
    cache     = new List<SectorCache>;
    semaphore = new Semaphore("cache disk sem",0);
}

CacheDisk::~CacheDisk()
{
    while(!cache->IsEmpty()){
        CacheRemove();
    }
    delete cache;
    delete disk;
    delete semaphore;
}

void
CacheDisk::ReadSector(int sectorNumber, char * data)
{
    ASSERT(data != nullptr);
    // SectorCache * sec = CacheAdd(sectorNumber);
    // memcpy(data,sec->data,SECTOR_SIZE);
    InternalRead(sectorNumber,data);
}

void
CacheDisk::WriteSector(int sectorNumber, const char * data)
{
    ASSERT(data != nullptr);
    // SectorCache * sec = CacheAdd(sectorNumber);
    // memcpy(sec->data,data,SECTOR_SIZE);
    // sec->modified = true;
    InternalWrite(sectorNumber,data);
}

//Pone una pagina al final de la cache(si no existe la trae de disco)
SectorCache *
CacheDisk::CacheAdd(int sectorNumber){
    SectorCache * s = NULL;
    if(!IsOnCache(sectorNumber,&s)){
        //Check Size
        size = 0;
        cache->Apply(aux_get_sz);
        if(size >= CACHE_SIZE){
            CacheRemove();
        }
        //Add sector
        s = new SectorCache;
        s->sector = sectorNumber;
        s->data = new char[SECTOR_SIZE];
        InternalRead(s->sector,s->data);
        s->modified = false;
        cache->Append(*s);
    }else{
        //Move sector to the end
        cache->Remove(*s);
        cache->Append(*s);
    }

    return s;
}

//Elimina una pagina de cache(y la escribe en disco de ser necesario)
void
CacheDisk::CacheRemove(){
    if(!cache->IsEmpty()){
        SectorCache s = cache->Pop();
        //Sincronizo de ser necesario
        if(s.modified){
            InternalWrite(s.sector,s.data);
        }
        delete s.data;
    }
}

//Busca una pagina en cache
bool
CacheDisk::IsOnCache(int sectorNumber,SectorCache ** Sec)
{
    sector_num = sectorNumber;
    *Sec = cache->GetFirst(aux_cmp_cache);
    return (*Sec!=NULL);
}

//Escriben y leen del disco directamente
void
CacheDisk::InternalWrite(int sectorNumber, const char * data){
    ASSERT(data != nullptr);
    disk->WriteRequest(sectorNumber, data);
    semaphore->P(); // Wait for interrupt.
}

void
CacheDisk::InternalRead(int sectorNumber, char * data)
{
    ASSERT(data != nullptr);
    disk->ReadRequest(sectorNumber, data);
    semaphore->P(); // Wait for interrupt.

}

void
CacheDisk::RequestDone()
{
    semaphore->V();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
