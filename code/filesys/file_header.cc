/// Routines for managing the disk file header (in UNIX, this would be called
/// the i-node).
///
/// The file header is used to locate where on disk the file's data is
/// stored.  We implement this as a fixed size table of pointers -- each
/// entry in the table points to the disk sector containing that portion of
/// the file data (in other words, there are no indirect or doubly indirect
/// blocks). The table size is chosen so that the file header will be just
/// big enough to fit in one disk sector,
///
/// Unlike in a real system, we do not keep track of file permissions,
/// ownership, last modification date, etc., in the file header.
///
/// A file header can be initialized in two ways:
///
/// * for a new file, by modifying the in-memory data structure to point to
///   the newly allocated data blocks;
/// * for a file already on disk, by reading the file header from disk.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "lib/utility.hh"
#include "file_header.hh"
#include "threads/system.hh"

/// Initialize a fresh file header for a newly created file.  Allocate data
/// blocks for the file out of the map of free disk blocks.  Return false if
/// there are not enough free blocks to accomodate the new file.
///
/// * `freeMap` is the bit map of free disk sectors.
/// * `fileSize` is the bit map of free disk sectors.

void
FileHeader::Get_Lock()
{
    if (sectornumber != NOT_ASSIGNED) {
        if (filetable->find(sectornumber) == nullptr) {
            filetable->add_file("FileHeader", sectornumber);
        }
        Filenode * node = filetable->find(sectornumber);
        node->File_Lock->Acquire();
        DEBUG('f', "Tomando Header Lock:%x Thread:%s Sector:%u\n",
          node->File_Lock, currentThread->GetName(), sectornumber);
        FetchFrom(sectornumber);
    }
}

void
FileHeader::Release_Lock()
{
    if (sectornumber != NOT_ASSIGNED) {
        ASSERT(filetable->find(sectornumber) != nullptr);
        Filenode * node = filetable->find(sectornumber);
        WriteBack(sectornumber);
        DEBUG('f', "Liberando Header Lock:%x Thread:%s Sector:%u\n",
          node->File_Lock, currentThread->GetName(), sectornumber);
        node->File_Lock->Release();
    }
}

bool
FileHeader::Allocate(Bitmap * freeMap, unsigned fileSize)
{
    ASSERT(freeMap != nullptr);
    DEBUG('f', "Alloque %u bytes\n", fileSize);
    // Limpio la estructura
    for (unsigned i = 0; i < NUM_DIRECT; i++) {
        raw.dataSectors[i] = NOT_ASSIGNED;
    }
    raw.unrefSectors = NOT_ASSIGNED;

    if (fileSize == 0) {
        // Creo que raw_file_header, pero sin bloques
        raw.numBytes   = 0;
        raw.numSectors = 0;
        return true;
    }

    raw.numBytes   = fileSize;
    raw.numSectors = DivRoundUp(fileSize, SECTOR_SIZE);
    if (freeMap->CountClear() < raw.numSectors) {
        DEBUG('f', "No hay suficiente espacio en el disco\n");
        return false; // Not enough space.
    }

    for (unsigned i = 0; i < min(raw.numSectors, NUM_DIRECT); i++) {
        raw.dataSectors[i] = freeMap->Find();
        synchDisk->ClearSector(raw.dataSectors[i]);
        DEBUG('F', "Tomo %u\n", raw.dataSectors[i]);
    }

    DEBUG('f', "raw.numSectors = %u\n", raw.numSectors);

    if (raw.numSectors > NUM_DIRECT) {
        // Dos niveles de indireccion (1 * 32 * 32 bytes)
        raw.unrefSectors = freeMap->Find();
        unsigned rest_sectors = raw.numSectors - NUM_DIRECT;
        unsigned * unref_lv1  = new unsigned[32];
        unsigned * unref_lv2  = new unsigned[32];
        // Limpio los arreglos
        for (unsigned k = 0; k < 32; k++)
            unref_lv1[k] = NOT_ASSIGNED;
        for (unsigned i = 0; i < 32 && 0 < rest_sectors; i++) {
            unref_lv1[i] = freeMap->Find();
            // Limpio el arreglo del segundo nivel
            for (unsigned k = 0; k < 32; k++)
                unref_lv2[k] = NOT_ASSIGNED;
            for (unsigned j = 0; j < 32 && 0 < rest_sectors; j++) {
                unref_lv2[j] = freeMap->Find();
                synchDisk->ClearSector(unref_lv2[j]);
                rest_sectors--;
            }
            DEBUG('f', "Sector %u\n", unref_lv1[i]);
            synchDisk->WriteSector(unref_lv1[i], (char *) unref_lv2);
        }
        DEBUG('f', "Sector %u\n", raw.unrefSectors);
        synchDisk->WriteSector(raw.unrefSectors, (char *) unref_lv1);

        return rest_sectors == 0;
    }

    for (unsigned i = 0; i < NUM_DIRECT; i++) {
        DEBUG('G', "Direct[%u] = %u\n", i, raw.dataSectors[i]);
    }
    DEBUG('G', "Unref = %u\n", raw.unrefSectors);
    raw.unrefSectors = NOT_ASSIGNED;

    return true;
} // FileHeader::Allocate

/// De-allocate all the space allocated for data blocks for this file.
///
/// * `freeMap` is the bit map of free disk sectors.
void
FileHeader::Deallocate(Bitmap * freeMap)
{
    ASSERT(freeMap != nullptr);
    for (unsigned i = 0; i < NUM_DIRECT; i++) {
        DEBUG('G', "Direct[%u] = %u\n", i, raw.dataSectors[i]);
    }
    // Borro los sectores directos
    for (unsigned i = 0; i < min(raw.numSectors, NUM_DIRECT); i++) {
        if (raw.dataSectors[i] != NOT_ASSIGNED) {
            ASSERT(freeMap->Test(raw.dataSectors[i]));
            DEBUG('G', "Liberando %u\n", raw.dataSectors[i]);
            freeMap->Clear(raw.dataSectors[i]);
            raw.dataSectors[i] = NOT_ASSIGNED;
        }
    }

    if (raw.unrefSectors != NOT_ASSIGNED) {
        // Borro los sectores de doble indireccion
        unsigned * unrf_lv1 = new unsigned[32];
        unsigned * unrf_lv2 = new unsigned[32];
        synchDisk->ReadSector(raw.unrefSectors, (char *) unrf_lv1);
        for (unsigned i = 0; i < 32; i++) {
            DEBUG('G', "Level1[%u] = %u\n", i, unrf_lv1[i]);
            if (unrf_lv1[i] != NOT_ASSIGNED && freeMap->Test(unrf_lv1[i])) {
                synchDisk->ReadSector(unrf_lv1[i], (char *) unrf_lv2);
                for (unsigned j = 0; j < 32; j++) {
                    DEBUG('G', "Level2[%u] = %u\n", j, unrf_lv2[j]);
                    if (unrf_lv2[j] != NOT_ASSIGNED &&
                      freeMap->Test(unrf_lv2[j]))
                    {
                        freeMap->Clear(unrf_lv2[j]);
                        unrf_lv2[j] = NOT_ASSIGNED;
                    }
                }
                freeMap->Clear(unrf_lv1[i]);
                unrf_lv1[i] = NOT_ASSIGNED;
            }
        }
        freeMap->Clear(raw.unrefSectors);
    }
    raw.unrefSectors = NOT_ASSIGNED;
    raw.numBytes     = 0;
    raw.numSectors   = 0;
} // FileHeader::Deallocate

/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
void
FileHeader::FetchFrom(unsigned sector)
{
    DEBUG('G', "\tReading %u\n", sector);
    sectornumber = sector;
    synchDisk->ReadSector(sector, (char *) this);
    // for(unsigned i = 0;i<NUM_DIRECT;i++){
    //  DEBUG('G',"Data[%u]:%u\n",i,raw.dataSectors[i]);
    // }
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void
FileHeader::WriteBack(unsigned sector)
{
    sectornumber = sector;
    synchDisk->WriteSector(sector, (char *) this);
    DEBUG('G', "\tWriting %u\n", sector);
    // for(unsigned i = 0;i<NUM_DIRECT;i++){
    //  DEBUG('G',"Data[%u]:%u\n",i,raw.dataSectors[i]);
    // }
}

/// Return which disk sector is storing a particular byte within the file.
/// This is essentially a translation from a virtual address (the offset in
/// the file) to a physical address (the sector where the data at the offset
/// is stored).
///
/// * `offset` is the location within the file of the byte in question.
unsigned
FileHeader::ByteToSector(unsigned offset)
{
    // Retorno el bloque correspondiente(Si no lo tengo, lo creo)
    DEBUG('f', "ByteToSector %u\n", offset);
    unsigned sector = offset / SECTOR_SIZE;
    // Me fijo si es un sector directo
    if (raw.numSectors < sector) {
        return NOT_ASSIGNED;
    }

    if (sector < NUM_DIRECT) {
        sector = raw.dataSectors[sector];
        return sector;
    }

    sector -= NUM_DIRECT;
    unsigned * unrf_lv1 = new unsigned[32];
    unsigned * unrf_lv2 = new unsigned[32];
    // Limpio los arreglos
    for (unsigned i = 0; i < 32; i++) {
        unrf_lv1[i] = unrf_lv2[i] = NOT_ASSIGNED;
    }
    synchDisk->ReadSector(raw.unrefSectors, (char *) unrf_lv1);
    synchDisk->ReadSector(unrf_lv1[sector / 32], (char *) unrf_lv2);

    sector = unrf_lv2[sector % 32];
    return sector;
} // FileHeader::ByteToSector

/// Return the number of bytes in the file.
unsigned
FileHeader::FileLength() const
{
    return raw.numSectors * SECTOR_SIZE;
}

/// Print the contents of the file header, and the contents of all the data
/// blocks pointed to by the file header.
void
FileHeader::Print()
{
    char * data = new char [SECTOR_SIZE];

    List<unsigned> * sectors_list = new List<unsigned>;

    printf("FileHeader contents.\n"
      "    Size: %u bytes\n"
      "    Block numbers: ",
      raw.numBytes);

    for (unsigned i = 0; i < min(raw.numSectors, NUM_DIRECT); i++) {
        printf("%u ", raw.dataSectors[i]);
        sectors_list->Append(raw.dataSectors[i]);
    }

    if (raw.numSectors > NUM_DIRECT) {
        ASSERT(raw.unrefSectors != NOT_ASSIGNED);
        unsigned * unrf_lv1 = new unsigned[32];
        unsigned * unrf_lv2 = new unsigned[32];
        synchDisk->ReadSector(raw.unrefSectors, (char *) unrf_lv1);
        for (unsigned i = 0; i < 32; i++) {
            if (unrf_lv1[i] != NOT_ASSIGNED) {
                synchDisk->ReadSector(unrf_lv1[i], (char *) unrf_lv2);
                for (unsigned j = 0; j < 32; j++) {
                    if (unrf_lv2[j] != NOT_ASSIGNED) {
                        printf("%u ", unrf_lv2[j]);
                        sectors_list->Append(unrf_lv2[j]);
                    }
                }
            }
        }
    }

    printf("\n    Contents:\n");
    unsigned bytes = 0;
    while (!sectors_list->IsEmpty()) {
        unsigned sector = sectors_list->Pop();
        synchDisk->ReadSector(sector, data);
        for (unsigned j = 0;
          j < SECTOR_SIZE && bytes < raw.numBytes;
          j++, bytes++)
        {
            if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
                printf("%c", data[j]);
            else
                printf("\\%X", (unsigned char) data[j]);
        }
        printf("\n");
    }
    delete [] data;
    delete sectors_list;
} // FileHeader::Print

const RawFileHeader *
FileHeader::GetRaw() const
{
    return &raw;
}

bool
FileHeader::Extend(Bitmap * freeMap, unsigned size)
{
    Get_Lock();
    // Retorno el bloque correspondiente(Si no lo tengo, lo creo)
    unsigned new_sectors     = DivRoundUp(size, SECTOR_SIZE);
    unsigned current_sectors = raw.numSectors;
    unsigned total_sectors   = new_sectors;

    if (current_sectors + new_sectors > NUM_DIRECT) {
        if (raw.unrefSectors == NOT_ASSIGNED)
            total_sectors++;
        total_sectors += DivRoundUp(new_sectors, (unsigned) 32);
    }

    if (freeMap->CountClear() < total_sectors) {
        Release_Lock();
        return false;
    }

    raw.numSectors += new_sectors;

    for (unsigned i = current_sectors; i < NUM_DIRECT && 0 < new_sectors; i++) {
        raw.dataSectors[i] = freeMap->Find();
        new_sectors--;
    }

    if (new_sectors == 0) {
        Release_Lock();
        return true;
    }

    // raw.unrefSectors -> [p0|p2|...|p31]
    //                      pN -> [b1|b2|...|b31]
    //                             bN -> [Sector con informacion del archivo]
    current_sectors -= NUM_DIRECT;
    unsigned * unrf_lv1 = new unsigned[32];
    unsigned * unrf_lv2 = new unsigned[32];
    // Limpio los arreglos
    for (unsigned i = 0; i < 32; i++) {
        unrf_lv1[i] = unrf_lv2[i] = NOT_ASSIGNED;
    }

    // Leo raw.unrefSectors -> [p0|p2|...|p31] (si no existe lo creo)
    if (raw.unrefSectors == NOT_ASSIGNED) {
        raw.unrefSectors = freeMap->Find();
    } else {
        synchDisk->ReadSector(raw.unrefSectors, (char *) unrf_lv1);
    }

    for (unsigned i = current_sectors / 32; i < 32 && 0 < new_sectors; i++) {
        if (unrf_lv1[i] == NOT_ASSIGNED) {
            unrf_lv1[i] = freeMap->Find();
            for (unsigned k = 0; k < 32; k++) {
                unrf_lv2[k] = NOT_ASSIGNED;
            }
        } else {
            synchDisk->ReadSector(unrf_lv1[i], (char *) unrf_lv2);
        }
        for (unsigned j = 0; j < 32 && 0 < new_sectors; j++) {
            if (unrf_lv2[j] == NOT_ASSIGNED) {
                unrf_lv2[j] = freeMap->Find();
                new_sectors--;
            }
        }
        synchDisk->WriteSector(unrf_lv1[i], (char *) unrf_lv2);
    }
    synchDisk->WriteSector(raw.unrefSectors, (char *) unrf_lv1);
    Release_Lock();
    return (new_sectors <= 0);
} // FileHeader::Extend
