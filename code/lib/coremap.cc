#include "coremap.hh"
#include "threads/system.hh"

unsigned CoreMap::phy_page_num=0;
unsigned CoreMap::vir_page_num=0;

bool operator ==(const PageContent& x,const PageContent& y){
	return x.vpn == y.vpn && x.ppn == y.ppn && x.thread == y.thread;
}

static bool cmp_ppn(PageContent pc){
    return pc.ppn == CoreMap::phy_page_num;
}

CoreMap::CoreMap()
{
    core_map = new List<PageContent>;
}

CoreMap::~CoreMap()
{
    delete core_map;
}

void
CoreMap::store(unsigned vpn)
{
    PageContent pc;
    pc.thread = currentThread;
    pc.vpn = vpn;
    pc.ppn = currentThread->space->pageTable[vpn].physicalPage;
    core_map->Append(pc);
}

bool
CoreMap::find(unsigned phy_page, PageContent* pc){
    CoreMap::phy_page_num = phy_page;
    pc = core_map->GetFirst(cmp_ppn);
    ASSERT(pc==NULL || pc->thread==currentThread);
    return pc != NULL;
}

void
CoreMap::remove(unsigned page){
    PageContent pc;
    if(find(page,&pc))
        core_map->Remove(pc);
}

int
CoreMap::freepage(){
    if(core_map->IsEmpty())return -3;
    PageContent pc = core_map->Pop();
    core_map->Remove(pc);
    pc.thread->space->save_page(pc.vpn);
    pc.thread->space->pageTable[pc.vpn].valid = false;
    return pc.ppn;
}
