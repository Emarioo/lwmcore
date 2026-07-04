#pragma once

#include <stdint.h>
#include <stdbool.h>


#define LWM_REGNR_PC 32
#define LWM_REGNR_SP 15
#define LWM_REGNR_LR 14
#define LWM_REGNR_TP 13

#define LWM_REGNR_CRSTATUS     0
#define LWM_REGNR_CRVB         1
#define LWM_REGNR_CRPT         2
#define LWM_REGNR_CREPC        3
#define LWM_REGNR_CRCAUSE      4
#define LWM_REGNR_CRFAULT      5
#define LWM_REGNR_CRCPUID      6
#define LWM_REGNR_CRTIMERCMP   7


#define CRSTATUS_USER        0x1
#define CRSTATUS_PAGING      0x2
#define CRSTATUS_INTERRUPT   0x4

// Instruction opcode numbers. Not strictly related to encoding
enum OpcodeKind {
    
    // [ opcode 3 | reg 5 | immediate ]
    OPCODE_LI8,
    OPCODE_LI16,
    OPCODE_LI32,
    OPCODE_LI64,
    // [ opcode 3 | reg 5 ]
    OPCODE_CALL_REG,
    OPCODE_JMP_REG,

    // [ opcode 6 | reg 5 | reg 5 ]
    OPCODE_NOT,
    OPCODE_MFCR,
    OPCODE_MTCR,
    OPCODE_CPUFEAT,

    // [ opcode 8 | reg 5 | reg 5 | reg 5 ]
    OPCODE_ADD,
    OPCODE_SUB,
    OPCODE_UMUL,
    OPCODE_UDIV,
    OPCODE_UMOD,
    OPCODE_SMUL,
    OPCODE_SDIV,
    OPCODE_SMOD,
    OPCODE_AND,
    OPCODE_OR,
    OPCODE_XOR,
    OPCODE_SHL,
    OPCODE_SHR,

    
    // [ opcode 8 | immediate ]
    OPCODE_JMP,
    OPCODE_JMP1,
    OPCODE_JMP2,
    OPCODE_CALL,
    OPCODE_CALL1,
    OPCODE_CALL2,

    // [ opcode 8 ]
    OPCODE_RET,
    OPCODE_SYSCALL,
    OPCODE_VRET,
    OPCODE_DBG,
    OPCODE_EINT,
    OPCODE_DINT,
    OPCODE_SLOW,
    OPCODE_WFI,
    OPCODE_NOP,

    // [ opcode 8 | flags 3 | reg 5 | relative8/16/32 ]
    OPCODE_JZ,
    OPCODE_JNZ,

    // [ opcode 8 | flags 6 | reg 5 | reg 5 | relative8/16/32 ]
    OPCODE_JCOND,

    // [ opcode 8 | reg 5 | reg 5 | reg 5 | disp8/16/32/64 ]

    OPCODE_LEA,
    OPCODE_LDB,
    OPCODE_LDBS,
    OPCODE_LDH,
    OPCODE_LDHS,
    OPCODE_LDL,
    OPCODE_LDLS,
    OPCODE_LDQ,
    OPCODE_STB,
    OPCODE_STH,
    OPCODE_STL,
    OPCODE_STQ,

    
    OPCODE_PUSH,
    OPCODE_POP,

    // Special instructions
    OPCODE_RDTICK,
    OPCODE_RDTICK1,
    OPCODE_RDTICK2,
    OPCODE_TLBFLUSH,

    OPCODE_VMSTART,
};

typedef enum {
    COND_EQ,
    COND_NE,
    COND_LT,
    COND_LE,
    COND_GT,
    COND_GE,
    COND_A,
    COND_AE,
    COND_B,
    COND_BE,
} ConditionKind;


typedef enum {
    ADDRESSING_ABS16,
    ADDRESSING_ABS32,
    ADDRESSING_ABS64,
    ADDRESSING_REG1_DISP8,
    ADDRESSING_REG1_DISP16,
    ADDRESSING_REG1_DISP32,
    ADDRESSING_REG2_DISP8,
    ADDRESSING_REG2_DISP16,
    ADDRESSING_REG2_DISP32,
    ADDRESSING_PC_DISP32,
} AddressingForm;

