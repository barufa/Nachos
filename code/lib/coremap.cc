#include "coremap.hh"
#include "threads/system.hh"

////////////////////////////////////////////////////////////////////////////////

static unsigned size             = 0;
static unsigned phy_page_num     = 0;
static AddressSpace * space_addr = nullptr;

bool
operator == (const PageContent& x, const PageContent& y)
{
    return x.vpn == y.vpn && x.ppn == y.ppn && x.space == y.space;
}

static bool
aux_cmp_ppn(PageContent pc)
{
    return pc.ppn == phy_page_num;
}

static bool
aux_cmp_space(PageContent pc)
{
    return pc.space == space_addr;
}

static void
aux_get_sz(PageContent pc)
{
    size++;
}

// ~ void aux_get_prt(PageContent pc){
// ~ DEBUG('W',"\t(%u,%u,%u)\n",pc.space,pc.vpn,pc.ppn);
// ~ }

////////////////////////////////////////////////////////////////////////////////

CoreMap::CoreMap()
{
    core_map = new List<PageContent>;
}

CoreMap::~CoreMap()
{
    delete core_map;
}

void
CoreMap::store(unsigned vpn, AddressSpace * space)
{
    ASSERT(currentThread->space == space);
    PageContent pc;
    pc.space = space;
    pc.vpn   = vpn;
    pc.ppn   = currentThread->space->pageTable[vpn].physicalPage;
    core_map->Append(pc);
}

bool
CoreMap::find(unsigned phy_page, PageContent * pc)
{
    return get_phy_page(phy_page, pc);
}

void
CoreMap::remove(unsigned page)
{
    PageContent pc;

    if (find(page, &pc)) {
        core_map->Remove(pc);
    }
}

void
CoreMap::freepage(void)
{
    ASSERT(!core_map->IsEmpty());
    PageContent pc = core_map->Pop();
    pc.space->save_page(pc.vpn);
}

unsigned
CoreMap::length(void)
{
    size = 0;
    core_map->Apply(aux_get_sz);
    return size;
}

void
CoreMap::access(unsigned page)
{
    PageContent pc;

    if (get_phy_page(page, &pc)) {
        core_map->Remove(pc);
        core_map->Append(pc);
    }
}

void
CoreMap::clean_space(AddressSpace * space)
{
    PageContent pc;

    while (get_space_addr(space, &pc)) {
        core_map->Remove(pc);
    }
}

////////////////////////////////////////////////////////////////////////////////

bool
CoreMap::get_phy_page(unsigned phy_page, PageContent * pc)
{
    phy_page_num = phy_page;
    PageContent * res;
    if ((res = core_map->GetFirst(aux_cmp_ppn)) != nullptr) {
        *pc = *res;
        return true;
    }
    return false;
}

bool
CoreMap::get_space_addr(AddressSpace * space, PageContent * pc)
{
    space_addr = space;
    PageContent * res;
    if ((res = core_map->GetFirst(aux_cmp_space)) != nullptr) {
        *pc = *res;
        return true;
    }
    return false;
}
