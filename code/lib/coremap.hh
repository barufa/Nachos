#ifndef NACHOS_LIB_COREMAP__HH
#define NACHOS_LIB_COREMAP__HH

#include "userprog/address_space.hh"
#include "filesys/open_file.hh"
#include "list.hh"

class Thread;

typedef struct {
    AddressSpace * space;
    unsigned       vpn;
    unsigned       ppn;
} PageContent;

class CoreMap {
public:
    CoreMap();
    ~CoreMap();
    // Almacena una pagina
    void
    store(unsigned vpn, AddressSpace * space);
    // Dada una pagina, devuelve el PageContent
    bool
    find(unsigned phy_page, PageContent * pc);
    // Borra una pagina fisica de la estructura
    void
    remove(unsigned page);
    // Nos da la proxima pagina victima
    void
    freepage();
    // Marca un acceso al coremap(implementando LRU)
    void
    access(unsigned page);
    // Cuenta la cantidad de elementos
    unsigned
    length(void);
    // Elimina todas las paginas relacionadas a un espacio
    void
    clean_space(AddressSpace * space);
private:
    List < PageContent > *core_map;
    bool
    get_phy_page(unsigned phy_page, PageContent * pc);
    bool
    get_space_addr(AddressSpace * space, PageContent * pc);
};


#endif /* ifndef NACHOS_LIB_COREMAP__HH */
