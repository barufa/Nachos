#ifndef NACHOS_LIB_FILETABLE__HH
#define NACHOS_LIB_FILETABLE__HH

#include "lib/list.hh"
#include "threads/synch.hh"

typedef struct Filenode {
    bool        remove;
    char *      name;
    unsigned    users = 0, sector = 0, lectores = 0;
    // Para leer y escribir de forma segura
	Lock * Dir_Lock,*File_Lock;
    Semaphore * Can_Write, * Can_Read;
    friend bool operator == (const Filenode &x, const Filenode &y){
        return x.sector == y.sector;
    }
} Filenode;

// Un archivo es identificado a partir del sector de su "inodo"
class FileTable {
public:
    FileTable();
    ~FileTable();
    // Agrega un archivo
    Filenode *
    add_file(const char * name, int sector);
    // Dado un sector, se fija si existe y retorna un puntero a el
    Filenode *
    find(int sector);
    // Elimina un archivo(sector) de la estructura, si existe
    void
    remove(int sector);
    // Retorna la cantidad de archivos almacenados
    unsigned
    length(void);
private:
    List < Filenode > *listmap;
};


#endif /* ifndef NACHOS_LIB_COREMAP__HH */
