/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2018 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_FILESYS_RAWFILEHEADER__HH
#define NACHOS_FILESYS_RAWFILEHEADER__HH


#include "machine/disk.hh"


const unsigned NUM_DIRECT    = (SECTOR_SIZE - 4 * sizeof(int)) / sizeof(int);
const unsigned MAX_FILE_SIZE = NUM_DIRECT * SECTOR_SIZE;
// NUM_DIRECT = 30 Sectores puede ser direccionados
// MAX_FILE_SIZE = 30 * 128 bytes posibles
// Necesito 1024 sectores para direccionar 128 Kb
// Si uso una indireccion 30 * 32 = 960 sectores(Aparentemente no son suficientes)
// Si uso dos indirecciones 29 + 1 * 32 * 32 son suficientes

struct RawFileHeader {
    unsigned unrefSectors;
    unsigned numBytes;                ///< Number of bytes in the file.
    unsigned numSectors;              ///< Number of data sectors in the file.
    unsigned dataSectors[NUM_DIRECT]; ///< Disk sector numbers for each data
    /// block in the file.
};


#endif /* ifndef NACHOS_FILESYS_RAWFILEHEADER__HH */
