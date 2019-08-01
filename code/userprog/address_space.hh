/// Data structures to keep track of executing user programs (address
/// spaces).
///
/// For now, we do not keep any information about address spaces.  The user
/// level CPU state is saved and restored in the thread executing the user
/// program (see `thread.hh`).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_USERPROG_ADDRESSSPACE__HH
#define NACHOS_USERPROG_ADDRESSSPACE__HH


#include "filesys/file_system.hh"
#include "machine/translation_entry.hh"
#include "../bin/noff.h"

const unsigned USER_STACK_SIZE = 1024;  ///< Increase this as necessary!


class AddressSpace {
public:

    /// Create an address space, initializing it with the program stored in
    /// the file `executable`.
    ///
    /// * `executable` is the open file that corresponds to the program.
    AddressSpace(OpenFile *executable);

    /// De-allocate an address space.
    ~AddressSpace();

    /// Initialize user-level CPU registers, before jumping to user code.
    void InitRegisters();

    /// Save/restore address space-specific info on a context switch.

    void SaveState();
    void RestoreState();
    bool Update_TLB(unsigned vpn);

    void save_page(unsigned vpn);
    void load_page(unsigned vpn,unsigned ppn);
    bool swap_find(unsigned vpn);

    /// Assume linear page table translation for now!
    TranslationEntry *pageTable;

private:

    static unsigned last_page;
    static unsigned next_id;

    /// Number of pages in the virtual address space.
    unsigned numPages;

    bool LoadPage(unsigned vpn);
    noffHeader noffH;
    OpenFile *executable;

    char * swap_id;
    OpenFile * swap;

};



#endif
