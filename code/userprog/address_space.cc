/// Routines to manage address spaces (executing user programs).
///
/// In order to run a user program, you must:
///
/// 1. Link with the `-N -T 0` option.
/// 2. Run `coff2noff` to convert the object file to Nachos format (Nachos
///    object code format is essentially just a simpler version of the UNIX
///    executable object code format).
/// 3. Load the NOFF file into the Nachos file system (if you have not
///    implemented the file system yet, you do not need to do this last
///    step).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "bin/noff.h"
#include "machine/endianness.hh"
#include "threads/system.hh"
#include "lib/utility.hh"

unsigned AddressSpace::last_page = 0;
unsigned AddressSpace::next_id   = 0;
// Valores altos para usar como flags
const unsigned IN_SWAP = 4294967294;

/// Do little endian to big endian conversion on the bytes in the object file
/// header, in case the file was generated on a little endian machine, and we
/// are re now running on a big endian machine.
static void
SwapHeader(noffHeader * noffH)
{
    ASSERT(noffH != nullptr);

    noffH->noffMagic              = WordToHost(noffH->noffMagic);
    noffH->code.size              = WordToHost(noffH->code.size);
    noffH->code.virtualAddr       = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr        = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size          = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr   = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr    = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size        = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr  = WordToHost(noffH->uninitData.inFileAddr);
}

/// Create an address space to run a user program.
///
/// Load the program from a file `executable`, and set everything up so that
/// we can start executing user instructions.
///
/// Assumes that the object code file is in NOFF format.
///
/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
///
/// * `executable` is the file containing the object code to load into
///   memory.

AddressSpace::AddressSpace(OpenFile * _executable)
{
    executable = _executable;

    ASSERT(executable != nullptr);

    executable->ReadAt((char *) &noffH, sizeof noffH, 0);
    if (noffH.noffMagic != NOFF_MAGIC &&
      WordToHost(noffH.noffMagic) == NOFF_MAGIC)
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFF_MAGIC);

    unsigned size = noffH.code.size + noffH.initData.size
      + noffH.uninitData.size + USER_STACK_SIZE;
    // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size     = numPages * PAGE_SIZE;

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
      numPages, size);

    NumPages = numPages;

    // First, set up the translation.
    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].physicalPage = NOT_ASSIGNED;
        pageTable[i].virtualPage  = i;
        pageTable[i].valid        = false;
        pageTable[i].use      = false;
        pageTable[i].dirty    = false;
        pageTable[i].readOnly = false;
        // If the code segment was entirely on a separate page, we could
        // set its pages to be read-only.
    }
    // Create swap
    swap_id = new char[20];
    sprintf(swap_id, "swap.%u", AddressSpace::next_id);
    fileSystem->Remove(swap_id);
    fileSystem->Create(swap_id, numPages * PAGE_SIZE);
    swap = fileSystem->Open(swap_id);
    ASSERT(swap);
    AddressSpace::next_id = (AddressSpace::next_id + 1) % 4096;
}

/// Deallocate an address space.
AddressSpace::~AddressSpace()
{
    DEBUG('a', "Liberando %u paginas\n", numPages);
    for (unsigned page = 0; page < numPages; page++) {
        unsigned ppn = pageTable[page].physicalPage;
        if (ppn != IN_SWAP && ppn != NOT_ASSIGNED)
            bitmap->Clear(pageTable[page].physicalPage);
    }
    fileSystem->Remove(swap_id);
    coremap->clean_space(this);
    delete [] pageTable;
    delete [] swap_id;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
      numPages * PAGE_SIZE - 16);
}

bool
AddressSpace::LoadPage(unsigned vpn)
{
    if (!pageTable[vpn].valid) {
        // Asigno una pagina en memoria
        DEBUG('a', "\tbitmap size: %u\n\tcoremap size: %u\n",
          bitmap->CountNotClear(), coremap->length());
        int next_page = bitmap->Find();
        if (next_page < 0) {
            coremap->freepage();
            next_page = bitmap->Find();
        }
        ASSERT(next_page >= 0);
        DEBUG('a', "Cargando Pagina %u en %d\n", vpn, next_page);
        if (swap_find(vpn)) {
            DEBUG('a', "Cargando desde swap\n");
            load_page(vpn, next_page);
        } else {
            DEBUG('a', "Cargando desde archivo\n");
            pageTable[vpn].physicalPage = next_page;
            // Copio la informacion a la pagina
            char * mainMemory   = machine->GetMMU()->mainMemory;
            uint32_t code_begin = noffH.code.virtualAddr;
            uint32_t code_end   = noffH.code.virtualAddr + noffH.code.size;
            uint32_t data_begin = noffH.initData.virtualAddr;
            uint32_t data_end   = noffH.initData.virtualAddr
              + noffH.initData.size;
            uint32_t VirtualAddr  = pageTable[vpn].virtualPage * PAGE_SIZE;
            uint32_t PhysicalAddr = pageTable[vpn].physicalPage * PAGE_SIZE;
            uint32_t inFileAddr   = 0;
            if (code_begin <= VirtualAddr && VirtualAddr <= code_end) {
                inFileAddr = noffH.code.inFileAddr + (VirtualAddr - code_begin);
            } else if (data_begin <= VirtualAddr && VirtualAddr <= data_end) {
                inFileAddr = noffH.initData.inFileAddr
                  + (VirtualAddr - data_begin);
            }
            memset(&mainMemory[PhysicalAddr], 0, PAGE_SIZE);
            executable->ReadAt(&(mainMemory[PhysicalAddr]), PAGE_SIZE,
              inFileAddr);
            pageTable[vpn].valid = true;
        }
        coremap->store(vpn, this);
    }
    return pageTable[vpn].valid;
} // AddressSpace::LoadPage

bool
AddressSpace::Update_TLB(unsigned vpn)
{
    // Verificar que la vpn sea valida
    if (numPages <= vpn) {
        DEBUG('a', "VPN=%u invalida\n", vpn);
        ASSERT(numPages > vpn);
    }
    DEBUG('a', "Agrego %u(%u) a la TLB %s\n", vpn, pageTable[vpn].physicalPage,
      pageTable[vpn].valid ? "y es valida" : "y no es valida");
    // Cargo la pagina en memoria
    if (!LoadPage(vpn)) {
        DEBUG('a', "LoadPage retorno false para %u\n", vpn);
    }
    // Busco pagina victima en TLB
    unsigned next     = (AddressSpace::last_page++) % TLB_SIZE;
    unsigned next_vpn = machine->GetMMU()->Get_Entry(next).virtualPage;

    // Guardo la pagina victima en memoria y actualizo la tlb
    if (machine->GetMMU()->Get_Entry(next).valid) {
        pageTable[next_vpn] = machine->GetMMU()->Get_Entry(next);
    }
    machine->GetMMU()->Set_Entry(pageTable[vpn], next);
    coremap->access(pageTable[vpn].physicalPage);
    DEBUG('a', "Swapeando %d(%d) con %d(%d)\n", vpn,
      pageTable[vpn].physicalPage, next_vpn,
      pageTable[next_vpn].physicalPage);
    return true;
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{
    machine->GetMMU()->Get_TLB(pageTable, numPages);
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
    machine->GetMMU()->Set_TLB(pageTable, numPages);
}

void
AddressSpace::save_page(unsigned vpn)
{
    TranslationEntry * page = &pageTable[vpn];
    bool dirty = false;

    for (unsigned i = 0; i < TLB_SIZE; i++) {
        if (machine->GetMMU()->Get_Entry(i).virtualPage == vpn) {
            *page       = machine->GetMMU()->Get_Entry(i);
            dirty       = page->dirty;
            page->valid = false;
            page->dirty = false;
            machine->GetMMU()->Set_Entry(*page, i);
            break;
        }
    }
    char * mainMemory     = machine->GetMMU()->mainMemory;
    uint32_t PhysicalAddr = page->physicalPage * PAGE_SIZE;

    if (dirty || !swap_find(vpn)) {
        swap->WriteAt(&mainMemory[PhysicalAddr], PAGE_SIZE, vpn * PAGE_SIZE);
        DEBUG('a', "Enviando %u a swap con %x\n", vpn,
          ((int *) mainMemory)[PhysicalAddr]);
    }

    bitmap->Clear(page->physicalPage);
    memset(&mainMemory[PhysicalAddr], 0, PAGE_SIZE);
    page->valid        = false;
    page->dirty        = false;
    page->physicalPage = IN_SWAP;
}

void
AddressSpace::load_page(unsigned vpn, unsigned ppn)
{
    ASSERT(swap_find(vpn));
    char * mainMemory     = machine->GetMMU()->mainMemory;
    uint32_t PhysicalAddr = ppn * PAGE_SIZE;
    swap->ReadAt(&mainMemory[PhysicalAddr], PAGE_SIZE, vpn * PAGE_SIZE);

    pageTable[vpn].valid        = true;
    pageTable[vpn].virtualPage  = vpn;
    pageTable[vpn].physicalPage = ppn;

    DEBUG('a', "Traigo de swap %u(%u) con %x\n", vpn, ppn,
      ((int *) mainMemory)[PhysicalAddr]);
}

bool
AddressSpace::swap_find(unsigned vpn)
{
    return pageTable[vpn].physicalPage == IN_SWAP;
}
