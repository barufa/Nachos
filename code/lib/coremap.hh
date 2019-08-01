#ifndef NACHOS_LIB_COREMAP__HH
#define NACHOS_LIB_COREMAP__HH

#include "filesys/open_file.hh"
#include "list.hh"

class Thread;

typedef struct{
	Thread * thread;
	unsigned vpn;
	unsigned ppn;
}PageContent;

class CoreMap {
public:
	CoreMap();
	~CoreMap();
	// Almacena una pagina
	void store(unsigned vpn);
	//Dada una pagina, devuelve el PageContent
	bool find(unsigned phy_page,PageContent* pc);
	//Borra una pagina fisica de la estructura
	void remove(unsigned page);
	// Nos da la proxima pagina victima
	int freepage();
	static unsigned phy_page_num;
private:
    List<PageContent>* core_map;
    static unsigned vir_page_num;
};


#endif
