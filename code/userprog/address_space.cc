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

unsigned AddressSpace::last_page=0;

/// Do little endian to big endian conversion on the bytes in the object file
/// header, in case the file was generated on a little endian machine, and we
/// are re now running on a big endian machine.
static void
SwapHeader(noffHeader *noffH)
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
    noffH->uninitData.virtualAddr =
      WordToHost(noffH->uninitData.virtualAddr);
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

AddressSpace::AddressSpace(OpenFile *executable)
{
    ASSERT(executable != nullptr);

    noffHeader noffH;
    executable->ReadAt((char *) &noffH, sizeof noffH, 0);
    if (noffH.noffMagic != NOFF_MAGIC &&
          WordToHost(noffH.noffMagic) == NOFF_MAGIC)
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFF_MAGIC);

    // How big is address space?

    unsigned size = noffH.code.size + noffH.initData.size
                    + noffH.uninitData.size + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;


    DEBUG('e', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

	ASSERT(numPages <= NUM_PHYS_PAGES);
	// Check we are not trying to run anything too big -- at least until we
	// have virtual memory.

    char *mainMemory = machine->GetMMU()->mainMemory;
    // First, set up the translation.
    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        int next_page = bitmap->Find();
        ASSERT(next_page!=-1);
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = next_page;
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
        memset(&(mainMemory[next_page*PAGE_SIZE]), 0, PAGE_SIZE);
        // If the code segment was entirely on a separate page, we could
        // set its pages to be read-only.
    }


    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.


    // Then, copy in the code and data segments into memory.
    if (noffH.code.size > 0) {
        uint32_t virtualAddr = noffH.code.virtualAddr;
        uint32_t inFileAddr  = noffH.code.inFileAddr;
        uint32_t codesize    = noffH.code.size;

        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
              virtualAddr, codesize);

        for(uint32_t i=0;i<codesize;i++){
            unsigned page   = (virtualAddr+i)/PAGE_SIZE;
	    unsigned offset = (virtualAddr+i)%PAGE_SIZE;
            uint32_t physicalAddr = pageTable[page].physicalPage*PAGE_SIZE;
            executable->ReadAt(&(mainMemory[physicalAddr+offset]), 1, inFileAddr+i);
        }
        // executable->ReadAt(&(mainMemory[virtualAddr]),size,inFileAddr);
    }
    if (noffH.initData.size > 0) {
        uint32_t virtualAddr = noffH.initData.virtualAddr;
        uint32_t inFileAddr  = noffH.initData.inFileAddr;
        uint32_t datasize    = noffH.initData.size;

        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
                virtualAddr,datasize);

        for(uint32_t i=0;i<datasize;i++){
            unsigned page = (virtualAddr+i)/PAGE_SIZE;
	    unsigned offset = (virtualAddr+i)%PAGE_SIZE;
	    uint32_t physicalAddr = pageTable[page].physicalPage*PAGE_SIZE;
            executable->ReadAt(&(mainMemory[physicalAddr+offset]), 1, inFileAddr+i);
        }

        // executable->ReadAt(&(mainMemory[noffH.initData.virtualAddr]),
        // noffH.initData.size, noffH.initData.inFileAddr);
    }

}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
	DEBUG('e',"Liberando %u paginas\n",numPages);
	for(unsigned page=0;page<numPages;page++)
		bitmap->Clear(pageTable[page].physicalPage);
	delete [] pageTable;
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
AddressSpace::Update_TLB(unsigned vpn){
  // Verificar que la vpn sea valida
  if(numPages <= vpn){
	  DEBUG('w',"VPN=%u invalida\n",vpn);
	  return false;
  }
  
  // Busco pagina victima
  unsigned next = (++AddressSpace::last_page)%TLB_SIZE;
  unsigned next_vpn = machine->GetMMU()->Get_Entry(next).virtualPage;

  DEBUG('w',"Swapeando %d con %d\n",vpn,next_vpn);
  // Guardo la pagina victima en memoria y actualizo la tlb
  pageTable[next_vpn] = machine->GetMMU()->Get_Entry(next);
  machine->GetMMU()->Set_Entry(pageTable[vpn],next);

  for(unsigned i=0;i<TLB_SIZE;i++)
    DEBUG('w',"TLB[%u] = %d\n",i,machine->GetMMU()->Get_Entry(i).virtualPage);

	return true;
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{
  machine->GetMMU()->Get_TLB(pageTable,numPages);
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
    machine->GetMMU()->Set_TLB(pageTable,numPages);
}
