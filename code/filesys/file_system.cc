/// Routines to manage the overall operation of the file system.  Implements
/// routines to map from textual file names to files.
///
/// Each file in the file system has:
/// * a file header, stored in a sector on disk (the size of the file header
///   data structure is arranged to be precisely the size of 1 disk sector);
/// * a number of data blocks;
/// * an entry in the file system directory.
///
/// The file system consists of several data structures:
/// * A bitmap of free disk sectors (cf. `bitmap.h`).
/// * A directory of file names and file headers.
///
/// Both the bitmap and the directory are represented as normal files.  Their
/// file headers are located in specific sectors (sector 0 and sector 1), so
/// that the file system can find them on bootup.
///
/// The file system assumes that the bitmap and directory files are kept
/// “open” continuously while Nachos is running.
///
/// For those operations (such as `Create`, `Remove`) that modify the
/// directory and/or bitmap, if the operation succeeds, the changes are
/// written immediately back to disk (the two files are kept open during all
/// this time).  If the operation fails, and we have modified part of the
/// directory and/or bitmap, we simply discard the changed version, without
/// writing it back to disk.
///
/// Our implementation at this point has the following restrictions:
///
/// * there is no synchronization for concurrent accesses;
/// * files have a fixed size, set when the file is created;
/// * files cannot be bigger than about 3KB in size;
/// * there is no hierarchical directory structure, and only a limited number
///   of files can be added to the system;
/// * there is no attempt to make the system robust to failures (if Nachos
///   exits in the middle of an operation that modifies the file system, it
///   may corrupt the disk).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2018 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_system.hh"
#include "directory.hh"
#include "directory_entry.hh"
#include "file_header.hh"
#include "threads/system.hh"
#include "machine/disk.hh"
#include "lib/bitmap.hh"

#include "string.h"
/// Sectors containing the file headers for the bitmap of free sectors, and
/// the directory of files.  These file headers are placed in well-known
/// sectors, so that they can be located on boot-up.
static const unsigned FREE_MAP_SECTOR  = 0;
static const unsigned DIRECTORY_SECTOR = 1;

/// Initial file sizes for the bitmap and directory; until the file system
/// supports extensible files, the directory size sets the maximum number of
/// files that can be loaded onto the disk.
static const unsigned FREE_MAP_FILE_SIZE  = NUM_SECTORS / BITS_IN_BYTE;
static const unsigned DIRECTORY_FILE_SIZE = sizeof(DirectoryEntry)
  * NUM_DIR_ENTRIES;

static const char * getName(const char * name){

    int pos = strlen(name)-1;

    //No deberia pasar, pero...
    if(name[pos]=='/'){
		pos--;
	}
	for(unsigned i = pos;0<=i;i--){
		if(name[i]=='/'){
			return name+i+1;
		}
	}
	return name;
}

static const char * getParent(const char * path){

    char * parent = new char[PATH_MAX_LEN];
    strncpy(parent,path,PATH_MAX_LEN);

	DEBUG('f',"Buscando padre de %s\n", path);

    int pos = strlen(parent)-1;
    if(parent[pos]=='/'){
		pos--;
	}

	for(unsigned i = pos;i>=0;i--){
		if(parent[i]=='/'){
            parent[i+1]='\0';
			return parent;
		}
	}
	ASSERT(false);
	return parent;
}

Directory *
FileSystem::OpenPath(const char * _path,int * _sector){

	OpenFile * dir_file;
	int sector = DIRECTORY_SECTOR;
	Directory * dir = new Directory(NUM_DIR_ENTRIES);
	char * path = new char[PATH_MAX_LEN], *p;
	strncpy(path,_path+1,strlen(_path)-1);
	p = path;
	DEBUG('f',"Abriendo %s\n", path);

	dir->FetchFrom(directoryFile);
	unsigned l = strlen(path);
	for(unsigned i = 0;i < l;i++){
		if(path[i]=='/'){
			path[i] = '\0';
			sector = dir->Find(p,true);
			if(sector==-1){
				DEBUG('f',"No existe %s en %s\n",p,_path);
				return NULL;
			}else{
				DEBUG('f',"Accediendo a directorio %s\n",p);
				dir_file = new OpenFile(sector);
				dir->FetchFrom(dir_file);
				delete dir_file;
			}
			p += i+1;
		}
	}
	*_sector = sector;

	delete path;
	return dir;
}

/// Initialize the file system.  If `format == true`, the disk has nothing on
/// it, and we need to initialize the disk to contain an empty directory, and
/// a bitmap of free sectors (with almost but not all of the sectors marked
/// as free).
///
/// If `format == false`, we just have to open the files representing the
/// bitmap and the directory.
///
/// * `format` -- should we initialize the disk?
FileSystem::FileSystem(bool format)
{
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        Bitmap * freeMap       = new Bitmap(NUM_SECTORS);
        Directory * directory  = new Directory(NUM_DIR_ENTRIES);
        FileHeader * mapHeader = new FileHeader;
        FileHeader * dirHeader = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FREE_MAP_SECTOR);
        freeMap->Mark(DIRECTORY_SECTOR);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHeader->Allocate(freeMap, FREE_MAP_FILE_SIZE));
        ASSERT(dirHeader->Allocate(freeMap, DIRECTORY_FILE_SIZE));

        // Flush the bitmap and directory `FileHeader`s back to disk.
        // We need to do this before we can `Open` the file, since open reads
        // the file header off of disk (and currently the disk has garbage on
        // it!).

        DEBUG('f', "Writing headers back to disk.\n");
        mapHeader->WriteBack(FREE_MAP_SECTOR);
        dirHeader->WriteBack(DIRECTORY_SECTOR);

        // OK to open the bitmap and directory files now.
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        directoryFile = new OpenFile(DIRECTORY_SECTOR);

        // Once we have the files “open”, we can write the initial version of
        // each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        freeMap->WriteBack(freeMapFile); // flush changes to disk
        directory->WriteBack(directoryFile);

        if (debug.IsEnabled('f')) {
            freeMap->Print();
            directory->Print();

            delete freeMap;
            delete directory;
            delete mapHeader;
            delete dirHeader;
        }
    } else {
        // If we are not formatting the disk, just open the files
        // representing the bitmap and directory; these are left open while
        // Nachos is running.
        freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        directoryFile = new OpenFile(DIRECTORY_SECTOR);
    }
}

FileSystem::~FileSystem()
{
    delete freeMapFile;
    delete directoryFile;
}

/// Create a file in the Nachos file system (similar to UNIX `create`).
/// Since we cannot increase the size of files dynamically, we have to give
/// Create the initial size of the file.
///
/// The steps to create a file are:
/// 1. Make sure the file does not already exist.
/// 2. Allocate a sector for the file header.
/// 3. Allocate space on disk for the data blocks for the file.
/// 4. Add the name to the directory.
/// 5. Store the new file header on disk.
/// 6. Flush the changes to the bitmap and the directory back to disk.
///
/// Return true if everything goes ok, otherwise, return false.
///
/// Create fails if:
/// * file is already in directory;
/// * no free space for file header;
/// * no free entry for file in directory;
/// * no free space for data blocks for the file.
///
/// Note that this implementation assumes there is no concurrent access to
/// the file system!
///
/// * `name` is the name of file to be created.
/// * `initialSize` is the size of file to be created.
bool
FileSystem::Create(const char * path, unsigned initialSize)
{
    ASSERT(path != nullptr);

	int sector, dir_sector = DIRECTORY_SECTOR;
    Directory * directory = OpenPath(path,&dir_sector);
    Bitmap * freeMap;
    FileHeader * header;
    bool success;
	const char * name = getName(path);

    DEBUG('f', "Creating file %s, size %u\n", name, initialSize);

	if (directory == nullptr ||
		directory->Find(name) != -1) {
		DEBUG('f',"No encuentra el directorio o el nombre ya existe\n");
        success = false; // File is already in directory.
    } else {
        freeMap = new Bitmap(NUM_SECTORS);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find(); // Find a sector to hold the file header.
        if (sector == -1) {
            success = false; // No free block for file header.
        } else if (!directory->Add(name, sector)) {
            success = false; // No space in directory.
        } else {
            header = new FileHeader;
            if (!header->Allocate(freeMap, initialSize)) {
                success = false; // No space on disk for data.
            } else {
                success = true;
                // Everthing worked, flush all changes back to disk.
                header->WriteBack(sector);
                freeMap->WriteBack(freeMapFile);
				if(dir_sector==DIRECTORY_SECTOR){
					directory->WriteBack(directoryFile);
				}else{
					OpenFile * f = new OpenFile(dir_sector);
					directory->WriteBack(f);
					delete f;
				}
            }
            delete header;
        }
        delete freeMap;
    }
    delete directory;
	if(success){
		DEBUG('f',"Archivo %s creado\n",path);
	}

    return success;
} // FileSystem::Create

/// Open a file for reading and writing.
///
/// To open a file:
/// 1. Find the location of the file's header, using the directory.
/// 2. Bring the header into memory.
///
/// * `name` is the text name of the file to be opened.
OpenFile *
FileSystem::Open(const char * path)
{
    ASSERT(path != nullptr);

	int sector, dir_sector;
	OpenFile * openFile   = nullptr;
    Directory * directory = OpenPath(path,&dir_sector);
	const char * name = getName(path);

	DEBUG('f', "Opening file %s en %s\n", name, path);
    sector = directory->Find(name);
    if (sector > 1) {// `name` was found in directory.
		Filenode * node = filetable->find(sector);
        if (node == nullptr)
            node = filetable->add_file(name, sector);
        if (node->remove == false) {
			node->users++;
            openFile = new OpenFile(sector);
        }
    }
    delete directory;
    return openFile; // Return null if not found.
}

/// Delete a file from the file system.
///
/// This requires:
/// 1. Remove it from the directory.
/// 2. Delete the space for its header.
/// 3. Delete the space for its data blocks.
/// 4. Write changes to directory, bitmap back to disk.
///
/// Return true if the file was deleted, false if the file was not in the
/// file system.
///
/// * `name` is the text name of the file to be removed.
bool
FileSystem::Remove(const char * path)
{
    ASSERT(path != nullptr);

	int sector, dir_sector;
    Directory * directory = OpenPath(path, &dir_sector);
	const char * name = getName(path);

    sector = directory->Find(name);
    if (sector < 0) {
        delete directory;
        return false; // file not found
    }
    // Si hay alguien usando el archivo, solo lo marco para eliminar
    Filenode * node = filetable->find(sector);
    if (node != nullptr && node->users != 0) {
        node->remove = true;
    } else {
        Bitmap * freeMap;
        FileHeader * fileHeader;

        fileHeader = new FileHeader;
        fileHeader->FetchFrom(sector);
		directory->Remove(name);

        freeMap = new Bitmap(NUM_SECTORS);
        freeMap->FetchFrom(freeMapFile);

        fileHeader->Deallocate(freeMap); // Remove data blocks.
        freeMap->Clear(sector);          // Remove header block.

        freeMap->WriteBack(freeMapFile);     // Flush to disk.


		if(dir_sector==DIRECTORY_SECTOR){
			directory->WriteBack(directoryFile); // Flush to disk.
		}else{
			OpenFile * f = new OpenFile(dir_sector);
			directory->WriteBack(f);
			delete f;
		}

        filetable->remove(sector);

        delete fileHeader;
        delete freeMap;
    }
    delete directory;

	DEBUG('f',"Se elimino el archivo\n");

    return true;
} // FileSystem::Remove

/// List all the files in the file system directory.
void
FileSystem::List()
{
    Directory * directory = new Directory(NUM_DIR_ENTRIES);

    directory->FetchFrom(directoryFile);
    directory->Get_List();
    delete directory;
}

static bool
AddToShadowBitmap(unsigned sector, Bitmap * map)
{
    ASSERT(map != nullptr);

    if (map->Test(sector)) {
        DEBUG('f', "Sector %u was already marked.\n", sector);
        return false;
    }
    map->Mark(sector);
    DEBUG('f', "Marked sector %u.\n", sector);
    return true;
}

static bool
CheckForError(bool value, const char * message)
{
    if (!value)
        DEBUG('f', message);
    return !value;
}

static bool
CheckSector(unsigned sector, Bitmap * shadowMap)
{
    bool error = false;

    error |= CheckForError(sector < NUM_SECTORS, "Sector number too big.\n");
    error |= CheckForError(AddToShadowBitmap(sector, shadowMap),
        "Sector number already used.\n");
    return error;
}

static bool
CheckFileHeader(const RawFileHeader * rh, unsigned num, Bitmap * shadowMap)
{
    ASSERT(rh != nullptr);

    bool error = false;

    DEBUG('f',
      "Checking file header %u.  File size: %u bytes, number of sectors: %u.\n",
      num, rh->numBytes, rh->numSectors);
    error |= CheckForError(rh->numSectors >= DivRoundUp(rh->numBytes,
        SECTOR_SIZE),
        "Sector count not compatible with file size.\n");
    error |= CheckForError(rh->numSectors < NUM_DIRECT,
        "Too many blocks.\n");
    for (unsigned i = 0; i < rh->numSectors; i++) {
        unsigned s = rh->dataSectors[i];
        error |= CheckSector(s, shadowMap);
    }
    return error;
}

static bool
CheckBitmaps(const Bitmap * freeMap, const Bitmap * shadowMap)
{
    bool error = false;

    for (unsigned i = 0; i < NUM_SECTORS; i++) {
        DEBUG('f', "Checking sector %u. Original: %u, shadow: %u.\n",
          i, freeMap->Test(i), shadowMap->Test(i));
        error |= CheckForError(freeMap->Test(i) == shadowMap->Test(i),
            "Inconsistent bitmap.");
    }
    return error;
}

static bool
CheckDirectory(const RawDirectory * rd, Bitmap * shadowMap)
{
    ASSERT(rd != nullptr);
    ASSERT(shadowMap != nullptr);

    bool error         = false;
    unsigned nameCount = 0;
    const char * knownNames[NUM_DIR_ENTRIES];

    for (unsigned i = 0; i < NUM_DIR_ENTRIES; i++) {
        DEBUG('f', "Checking direntry: %u.\n", i);
        const DirectoryEntry * e = &rd->table[i];

        if (e->inUse) {
            if (strlen(e->name) > FILE_NAME_MAX_LEN) {
                DEBUG('f', "Filename too long.\n");
                error = true;
            }

            // Check for repeated filenames.
            DEBUG('f', "Checking for repeated names.  Name count: %u.\n",
              nameCount);
            bool repeated = false;
            for (unsigned j = 0; j < nameCount; j++) {
                DEBUG('f', "Comparing \"%s\" and \"%s\".\n",
                  knownNames[j], e->name);
                if (strcmp(knownNames[j], e->name) == 0) {
                    DEBUG('f', "Repeated filename.\n");
                    repeated = true;
                    error    = true;
                }
            }
            if (!repeated) {
                knownNames[nameCount] = e->name;
                DEBUG('f', "Added \"%s\" at %u.\n", e->name, nameCount);
                nameCount++;
            }

            // Check sector.
            error |= CheckSector(e->sector, shadowMap);

            // Check file header.
            FileHeader * h = new FileHeader;
            const RawFileHeader * rh = h->GetRaw();
            h->FetchFrom(e->sector);
            error |= CheckFileHeader(rh, e->sector, shadowMap);
            delete h;
        }
    }
    return error;
} // CheckDirectory

bool
FileSystem::Check()
{
    DEBUG('f', "Performing filesystem check\n");
    bool error = false;

    Bitmap * shadowMap = new Bitmap(NUM_SECTORS);
    shadowMap->Mark(FREE_MAP_SECTOR);
    shadowMap->Mark(DIRECTORY_SECTOR);

    DEBUG('f', "Checking bitmap's file header.\n");

    FileHeader * bitH = new FileHeader;
    const RawFileHeader * bitRH = bitH->GetRaw();
    bitH->FetchFrom(FREE_MAP_SECTOR);
    DEBUG('f', "  File size: %u bytes, expected %u bytes.\n"
      "  Number of sectors: %u, expected %u.\n",
      bitRH->numBytes, FREE_MAP_FILE_SIZE,
      bitRH->numSectors, FREE_MAP_FILE_SIZE / SECTOR_SIZE);
    error |= CheckForError(bitRH->numBytes == FREE_MAP_FILE_SIZE,
        "Bad bitmap header: wrong file size.\n");
    error |= CheckForError(
        bitRH->numSectors == FREE_MAP_FILE_SIZE / SECTOR_SIZE,
        "Bad bitmap header: wrong number of sectors.\n");
    error |= CheckFileHeader(bitRH, FREE_MAP_SECTOR, shadowMap);
    delete bitH;

    DEBUG('f', "Checking directory.\n");

    FileHeader * dirH = new FileHeader;
    const RawFileHeader * dirRH = dirH->GetRaw();
    dirH->FetchFrom(DIRECTORY_SECTOR);
    error |= CheckFileHeader(dirRH, DIRECTORY_SECTOR, shadowMap);
    delete dirH;

    Bitmap * freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(freeMapFile);
    Directory * dir = new Directory(NUM_DIR_ENTRIES);
    const RawDirectory * rdir = dir->GetRaw();
    dir->FetchFrom(directoryFile);
    error |= CheckDirectory(rdir, shadowMap);
    delete dir;

    // The two bitmaps should match.
    DEBUG('f', "Checking bitmap consistency.\n");
    error |= CheckBitmaps(freeMap, shadowMap);
    delete shadowMap;
    delete freeMap;

    DEBUG('f', error ? "Filesystem check succeeded.\n" :
      "Filesystem check failed.\n");

    return !error;
} // FileSystem::Check

/// Print everything about the file system:
/// * the contents of the bitmap;
/// * the contents of the directory;
/// * for each file in the directory:
///   * the contents of the file header;
///   * the data in the file.
void
FileSystem::Print()
{
    FileHeader * bitHeader = new FileHeader;
    FileHeader * dirHeader = new FileHeader;
    Bitmap * freeMap       = new Bitmap(NUM_SECTORS);
    Directory * directory  = new Directory(NUM_DIR_ENTRIES);

    printf("--------------------------------\n"
      "Bit map file header:\n\n");
    bitHeader->FetchFrom(FREE_MAP_SECTOR);
    bitHeader->Print();

    printf("--------------------------------\n"
      "Directory file header:\n\n");
    dirHeader->FetchFrom(DIRECTORY_SECTOR);
    dirHeader->Print();

    printf("--------------------------------\n");
    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    printf("--------------------------------\n");
    directory->FetchFrom(directoryFile);
    directory->Print();
    printf("--------------------------------\n");

    delete bitHeader;
    delete dirHeader;
    delete freeMap;
    delete directory;
}

bool
FileSystem::Expand(unsigned sector, unsigned size)
{
    FileHeader * header = new FileHeader;
    Bitmap * freeMap    = new Bitmap(NUM_SECTORS);
    bool ret = false;

    header->FetchFrom(sector);
    freeMap->FetchFrom(freeMapFile);
    if (header->Extend(freeMap, size)) {
        freeMap->WriteBack(freeMapFile);
        header->WriteBack(sector);
        ret = true;
    }
    delete freeMap;
    delete header;
    return ret;
}

bool
FileSystem::MakeDir(const char * path){
	ASSERT(path);

	int dir_sector, sector;
	FileHeader *header;
	Bitmap * freeMap;
	const char * parent_path = getParent(path);
	const char * name = getName(path);
	Directory * directory = OpenPath(parent_path,&dir_sector);
	bool success = true;

	DEBUG('f',"Creando el directorio %s en %s\n",name,parent_path);

	directory->Get_List();

	if(directory->Find(name,true)!=-1 || directory->Find(name,false)!=-1){
		DEBUG('f',"El directorio %s ya existe\n",name);
		delete directory;
		return false;
	}

	freeMap = new Bitmap(NUM_SECTORS);
	freeMap->FetchFrom(freeMapFile);
	sector = freeMap->Find(); // Find a sector to hold the file header.
	if (sector == -1) {
		DEBUG('f',"No hay sufuciente espacio en el disco\n");
		success = false; // No free block for file header.
	} else if (!directory->Add(name, sector, true)) {
		success = false; // No space in directory.
	} else {
		header = new FileHeader;
		if (!header->Allocate(freeMap, DIRECTORY_FILE_SIZE)) {
			success = false; // No space on disk for data.
			directory->Remove(name);
			if(freeMap->Test(sector)){
				freeMap->Clear(sector);
			}
		} else {
			success = true;
			OpenFile * new_file = new OpenFile(sector);
			Directory * new_dir = new Directory(NUM_DIR_ENTRIES);
			// new_dir->FetchFrom(new_file);
			new_dir->Clean(freeMap);
			new_dir->WriteBack(new_file);
			delete new_dir;
			delete new_file;
			// Everthing worked, flush all changes back to disk.
			header->WriteBack(sector);
			freeMap->WriteBack(freeMapFile);
			if(dir_sector==DIRECTORY_SECTOR){
				directory->WriteBack(directoryFile);
			}else{
				OpenFile * f = new OpenFile(dir_sector);
				directory->WriteBack(f);
				delete f;
			}
		}
		delete header;
	}
	delete freeMap;
	delete directory;

	printf("Saliendo de MakeDir\n");

	return success;
}

bool
FileSystem::RemoveDir(const char * path){

	ASSERT(path);
    if(strcmp(path,"/")==0){
		return false;
	}

    int dir_sector;
	Bitmap * freeMap;
    const char * name = getName(path);
    const char * parent_path = getParent(path);
	Directory * directory = OpenPath(path,&dir_sector);

    DEBUG('f',"Eliminando el directorio %s y su contenido\n",path);


    freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(freeMapFile);

	directory->Clean(freeMap);

	directory = OpenPath(parent_path,&dir_sector);
    directory->Remove(name);

    if(dir_sector==DIRECTORY_SECTOR){// Flush to disk.
        directory->WriteBack(directoryFile);
    }else{
        OpenFile * f = new OpenFile(dir_sector);
        directory->WriteBack(f);
        delete f;
    }
    freeMap->WriteBack(freeMapFile);// Flush to disk.

    delete freeMap;
    delete directory;

	return true;
}
