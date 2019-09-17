#include "filetable.hh"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
static unsigned aux_sector = 0;
static unsigned aux_size   = 0;

static bool
aux_cmp_sector(Filenode n)
{
    return n.sector == aux_sector;
}

static void
aux_count(Filenode n)
{
    aux_size++;
}

///////////////////////////////////////////////////////////////////////////////

FileTable::FileTable()
{
    listmap = new List<Filenode>;
}

FileTable::~FileTable()
{
    delete listmap;
}

Filenode *
FileTable::add_file(const char * _name, int k)
{
    Filenode v;

    v.name = new char[strlen(_name) + 2];
    strcpy(v.name, _name);
    v.remove    = false;
    v.users     = v.lectores = 0;
    v.sector    = k;
    v.Can_Read  = new Semaphore("Can_Read", 1);
    v.Can_Write = new Semaphore("Can_Write", 1);
    listmap->Prepend(v);
    return find(k);
}

Filenode *
FileTable::find(int k)
{
    aux_sector = k;
    return listmap->GetFirst(aux_cmp_sector);
}

void
FileTable::remove(int k)
{
    Filenode * node = find(k);

    if (node != nullptr) {
        delete [] (node->name);
        delete (node->Can_Read);
        delete (node->Can_Write);
        listmap->Remove(*node);
    }
}

unsigned
FileTable::length(void)
{
    aux_size = 0;
    listmap->Apply(aux_count);
    return aux_size;
}
