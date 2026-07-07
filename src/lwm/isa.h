#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


#define LWM_REGNR_TP 13
#define LWM_REGNR_LR 14
#define LWM_REGNR_SP 15
#define LWM_REGNR_PC 32

typedef enum {
    LWM_REGNR_CRSTATUS,
    LWM_REGNR_CRVB,
    LWM_REGNR_CRPT,
    LWM_REGNR_CRISP,

    LWM_REGNR_CRESTATUS,
    LWM_REGNR_CREPC,
    LWM_REGNR_CRESP,
    LWM_REGNR_CRCAUSE,
    LWM_REGNR_CRFAULT,

    LWM_REGNR_CRCPUID,
    LWM_REGNR_CRTIMERCMP,
    LWM_REGNR_CRKERNEL,
} ControlRegister;


#define CRSTATUS_USER        ((size_t)0x2)
#define CRSTATUS_PAGING      ((size_t)0x4)
#define CRSTATUS_INTERRUPT   ((size_t)0x8)

#define CRCAUSE_PRESENT      ((size_t)0x1)
#define CRCAUSE_USER         ((size_t)0x2)
#define CRCAUSE_READ         ((size_t)0x4)
#define CRCAUSE_WRITE        ((size_t)0x8)
#define CRCAUSE_EXECUTE      ((size_t)0x10)

#define PAGE_BIT_PRESENT     ((size_t)0x1)
#define PAGE_BIT_USER        ((size_t)0x2)
#define PAGE_BIT_READ        ((size_t)0x4)
#define PAGE_BIT_WRITE       ((size_t)0x8)
#define PAGE_BIT_EXECUTE     ((size_t)0x10)


#define VECTOR_ILLEGAL_INSTRUCTION  1
#define VECTOR_PROTECTION_FAULT     2
#define VECTOR_PAGE_FAULT           3
#define VECTOR_DOUBLE_FAULT         4
#define VECTOR_MISALIGNED           5
#define VECTOR_SYSCALL              6
#define VECTOR_DIVISION_BY_ZERO     7
#define VECTOR_BREAKPOINT           8
#define VECTOR_TIMER_INTERRUPT      9
#define VECTOR_BUS_ERROR            10
#define VECTOR_FATAL_MACHINE_ERROR  11

#define MAX_EXCEPTION_VECTORS 32
#define MAX_VECTORS 64

// Instruction opcode numbers. Not strictly related to encoding
typedef enum {
    
    // [ opcode 3 | reg 5 | immediate ]
    OPCODE_LI8,
    OPCODE_LI16,
    OPCODE_LI32,
    OPCODE_LI64,
    // [ opcode 3 | reg 5 ]
    OPCODE_CALL_REG,
    OPCODE_JMP_REG,
    OPCODE_PUSH,
    OPCODE_POP,
    OPCODE_TLBFLUSH,

    // [ opcode 6 | reg 5 | reg 5 ]
    OPCODE_NOT,
    OPCODE_MFCR,
    OPCODE_MTCR,
    OPCODE_MSCR,
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
    OPCODE_VMSTART,

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
    // OPCODE_XADD,

    // [ opcode 8 | reg 5 | immediate8 ]
    OPCODE_SAVE,
    OPCODE_RESTORE,

    // [ opcode 8 |  ]
    // OPCODE_CAS,
    // OPCODE_FENCE; // Add later, useful for real CPUs but not so much for emulator or logic world.

    // Special instructions
    OPCODE_RDTICK,
    OPCODE_RDTICK1,
    OPCODE_RDTICK2,
} OpcodeKind;

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


#define ADDRESSING_MASK_PRI(X)    ((X) & 0xF)
#define ADDRESSING_MASK_SEC(X)    (((X) >> 4) & 0xF)
#define ADDRESSING_ENUM(PRI, SEC) (((SEC) << 4) | (PRI))
typedef enum {
    // You can use ABS16/32/64 if register REG1 is 0.
    // It's rare to access absolute addresses so it seems reasonable.
    // Leaves room for more addessing modes.
    ADDRESSING_ABS16          = 0x00,
    ADDRESSING_ABS32          = 0x01,
    ADDRESSING_ABS64          = 0x02,
    ADDRESSING_PC_DISP8       = 0x03,
    ADDRESSING_PC_DISP16      = 0x04,
    ADDRESSING_PC_DISP32      = 0x05,
    ADDRESSING_REG1_DISP8     = 0x06,
    ADDRESSING_REG1_DISP16    = 0x16,
    ADDRESSING_REG1_DISP32    = 0x26,
    ADDRESSING_REG1_DISP64    = 0x36,
    ADDRESSING_REG1_PC_DISP8  = 0x46,
    ADDRESSING_REG1_PC_DISP16 = 0x56,
    ADDRESSING_REG1_PC_DISP32 = 0x66,
    ADDRESSING_REG2_DISP8     = 0x07,
    ADDRESSING_REG2_DISP16    = 0x17,
    ADDRESSING_REG2_DISP32    = 0x27,
    ADDRESSING_REG2_DISP64    = 0x37,
} AddressingForm;


int largest_encoding_ext(int opcode, AddressingForm form, int* lowestBytes);
static inline int largest_encoding(int opcode, AddressingForm form) {
    return largest_encoding_ext(opcode, form, NULL);
}

void gpr_to_string(int reg, char regname[20]);
void cr_to_string(int reg, char regname[20]);
const char* opcode_to_string(int opcode);
