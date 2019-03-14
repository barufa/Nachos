/// Internal data structures for simulating the MIPS instruction set.
///
/// DO NOT CHANGE -- part of the machine emulation
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_MACHINE_ENCODING__HH
#define NACHOS_MACHINE_ENCODING__HH


/// OpCode values.
///
/// The names are straight from the MIPS manual except for the following
/// special ones:
///
/// `OP_UNIMP`
///     Means that this instruction is legal, but has not been implemented
///     in the simulator yet.
/// `OP_RES`
///     Means that this is a reserved opcode (it is not supported by the
///     architecture).

#define OP_ADD       1
#define OP_ADDI      2
#define OP_ADDIU     3
#define OP_ADDU      4
#define OP_AND       5
#define OP_ANDI      6
#define OP_BEQ       7
#define OP_BGEZ      8
#define OP_BGEZAL    9
#define OP_BGTZ     10
#define OP_BLEZ     11
#define OP_BLTZ     12
#define OP_BLTZAL   13
#define OP_BNE      14

#define OP_DIV      16
#define OP_DIVU     17
#define OP_J        18
#define OP_JAL      19
#define OP_JALR     20
#define OP_JR       21
#define OP_LB       22
#define OP_LBU      23
#define OP_LH       24
#define OP_LHU      25
#define OP_LUI      26
#define OP_LW       27
#define OP_LWL      28
#define OP_LWR      29

#define OP_MFHI     31
#define OP_MFLO     32

#define OP_MTHI     34
#define OP_MTLO     35
#define OP_MULT     36
#define OP_MULTU    37
#define OP_NOR      38
#define OP_OR       39
#define OP_ORI      40
#define OP_RFE      41
#define OP_SB       42
#define OP_SH       43
#define OP_SLL      44
#define OP_SLLV     45
#define OP_SLT      46
#define OP_SLTI     47
#define OP_SLTIU    48
#define OP_SLTU     49
#define OP_SRA      50
#define OP_SRAV     51
#define OP_SRL      52
#define OP_SRLV     53
#define OP_SUB      54
#define OP_SUBU     55
#define OP_SW       56
#define OP_SWL      57
#define OP_SWR      58
#define OP_XOR      59
#define OP_XORI     60
#define OP_SYSCALL  61
#define OP_UNIMP    62
#define OP_RES      63
#define MAX_OPCODE  63

/// Miscellaneous definitions.

#define IndexToAddr(x)  ((x) << 2)

#define SIGN_BIT  0x80000000
#define R31       31

/// The table below is used to translate bits 31:26 of the instruction into a
/// value suitable for the `opCode` field of a `MemWord` structure, or into a
/// special value for further decoding.

#define SPECIAL  100
#define BCOND    101

#define IFMT  1
#define JFMT  2
#define RFMT  3

struct OpInfo {
    int opCode;  ///< Translated op code.
    int format;  ///< Format type (IFMT or JFMT or RFMT).
};

extern const OpInfo OP_TABLE[];

/// This table is used to convert the `funct` field of SPECIAL instructions
/// into the `opCode` field of a `MemWord`.
extern const int SPECIAL_TABLE[];


/// Stuff to help print out each instruction, for debugging.

enum RegType {
    NONE,
    RS,
    RT,
    RD,
    EXTRA
};

struct OpString {
    const char *string;  ///< Printed version of instruction.
    RegType     args[3];
};

extern const struct OpString OP_STRINGS[];


#endif
