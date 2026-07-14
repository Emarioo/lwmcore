
#include "lwm/isa.h"

#include <string.h>
#include <stdlib.h>

#include "lwm/util.h"

void gpr_to_string(int reg, char regname[20]) {
    if (reg == LWM_REGNR_LR)      sprintf(regname, "lr");
    else if (reg == LWM_REGNR_SP) sprintf(regname, "sp");
    else if (reg == LWM_REGNR_TP) sprintf(regname, "tp");
    else                          sprintf(regname, "r%d", reg);
}
void cr_to_string(int reg, char regname[20]) {
    // @TODO Make function to convert regnum to string
    switch ((ControlRegister)reg) {
        case LWM_REGNR_CRSTATUS:   sprintf(regname, "CRSTATUS"); return;
        case LWM_REGNR_CRVB:       sprintf(regname, "CRVB"); return;
        case LWM_REGNR_CRPT:       sprintf(regname, "CRPT"); return;
        case LWM_REGNR_CRISP:      sprintf(regname, "CRISP"); return;

        case LWM_REGNR_CRESTATUS:  sprintf(regname, "CRESTATUS"); return;
        case LWM_REGNR_CREPC:      sprintf(regname, "CREPC"); return;
        case LWM_REGNR_CRESP:      sprintf(regname, "CRESP"); return;
        case LWM_REGNR_CRCAUSE:    sprintf(regname, "CRCAUSE"); return;
        case LWM_REGNR_CRFAULT:    sprintf(regname, "CRFAULT"); return;

        case LWM_REGNR_CRCPUID:    sprintf(regname, "CRCPUID"); return;
        case LWM_REGNR_CRTIMERCMP: sprintf(regname, "CRTIMERCMP"); return;
        case LWM_REGNR_CRKERNEL:   sprintf(regname, "CRKERNEL"); return;
    }
    sprintf(regname, "cr%d", reg);
}

int largest_encoding_ext(int opcode, AddressingForm form, int* lowestBytes) {

    switch ((OpcodeKind)opcode) {
        case OPCODE_LI8:        return 3;
        case OPCODE_LI16:       return 4;
        case OPCODE_LI32:       return 6;
        case OPCODE_LI64:       *lowestBytes = 3; return 10;
        case OPCODE_CALL_REG:
        case OPCODE_JMP_REG:
        case OPCODE_PUSH:
        case OPCODE_POP:
        case OPCODE_TLBFLUSH:
            return 2;
        case OPCODE_NOT:
        case OPCODE_MFCR:
        case OPCODE_MTCR:
        case OPCODE_MSCR:
        case OPCODE_CPUFEAT:
            return 3;
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_UMUL:
        case OPCODE_UDIV:
        case OPCODE_UMOD:
        case OPCODE_SMUL:
        case OPCODE_SDIV:
        case OPCODE_SMOD:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_SHL:
        case OPCODE_SHR:
            return 4;
        case OPCODE_JMP:
        case OPCODE_JMP1:
        case OPCODE_JMP2:
        case OPCODE_CALL:
        case OPCODE_CALL1:
        case OPCODE_CALL2:
            if (lowestBytes) *lowestBytes = 2;
            return 5;
        case OPCODE_RET:
        case OPCODE_SYSCALL:
        case OPCODE_VRET:
        case OPCODE_DBG:
        case OPCODE_EINT:
        case OPCODE_DINT:
        case OPCODE_SLOW:
        case OPCODE_WFI:
        case OPCODE_NOP:
        case OPCODE_VMSTART:
            return 1;
        case OPCODE_JZ:
        case OPCODE_JNZ:
            return 6;
        case OPCODE_JCOND:
            return 7;
        case OPCODE_LEA:
        case OPCODE_LDB:
        case OPCODE_LDBS:
        case OPCODE_LDH:
        case OPCODE_LDHS:
        case OPCODE_LDL:
        case OPCODE_LDLS:
        case OPCODE_LDQ:
        case OPCODE_STB:
        case OPCODE_STH:
        case OPCODE_STL:
        case OPCODE_STQ:
        case OPCODE_XADD:
        case OPCODE_CAS:
            int baseBytes = opcode == OPCODE_CAS ? 3 : 2;
            switch (form) {
                case ADDRESSING_ABS16:          return baseBytes + 2;
                case ADDRESSING_ABS32:          return baseBytes + 4;
                case ADDRESSING_ABS64:          return baseBytes + 8;
                case ADDRESSING_REG1_DISP8:   
                case ADDRESSING_REG1_PC_DISP8:  return baseBytes + 2;
                case ADDRESSING_REG1_DISP16:    
                case ADDRESSING_REG1_PC_DISP16: return baseBytes + 3;
                case ADDRESSING_REG1_DISP32:    
                case ADDRESSING_REG1_PC_DISP32: if (lowestBytes) *lowestBytes = baseBytes + 2; return baseBytes + 5;
                case ADDRESSING_REG1_DISP64:    return baseBytes + 9;
                case ADDRESSING_REG2_DISP8:     return baseBytes + 3;
                case ADDRESSING_REG2_DISP16:    return baseBytes + 4;
                case ADDRESSING_REG2_DISP32:    return baseBytes + 6;
                case ADDRESSING_REG2_DISP64:    return baseBytes + 10;
                case ADDRESSING_PC_DISP8:       return baseBytes + 1;
                case ADDRESSING_PC_DISP16:      return baseBytes + 2;
                case ADDRESSING_PC_DISP32:      if (lowestBytes) *lowestBytes = baseBytes + 1; return baseBytes + 4;
            }
        case OPCODE_SAVE:
        case OPCODE_RESTORE: return 3;
        case OPCODE_RDTICK: return 2;
        case OPCODE_RDTICK1: return 3;
        case OPCODE_RDTICK2: return 5;
    }
    Assert(false);
    return -1;
}


const char* opcode_to_string(int opcode) {
    #define CASE(X, N) case X: return N;
    switch ((OpcodeKind)opcode) {
        CASE(OPCODE_LI8, "li8")
        CASE(OPCODE_LI16, "li16")
        CASE(OPCODE_LI32, "li32")
        CASE(OPCODE_LI64, "li64")
        CASE(OPCODE_CALL_REG, "call_reg")
        CASE(OPCODE_JMP_REG, "jmp_reg")
        CASE(OPCODE_PUSH, "push")
        CASE(OPCODE_POP, "pop")
        CASE(OPCODE_TLBFLUSH, "tlbflush")
        CASE(OPCODE_NOT, "not")
        CASE(OPCODE_MFCR, "mfcr")
        CASE(OPCODE_MTCR, "mtcr")
        CASE(OPCODE_MSCR, "mscr")
        CASE(OPCODE_CPUFEAT, "cpufeat")
        CASE(OPCODE_ADD, "add")
        CASE(OPCODE_SUB, "sub")
        CASE(OPCODE_UMUL, "umul")
        CASE(OPCODE_UDIV, "udiv")
        CASE(OPCODE_UMOD, "umod")
        CASE(OPCODE_SMUL, "smul")
        CASE(OPCODE_SDIV, "sdiv")
        CASE(OPCODE_SMOD, "smod")
        CASE(OPCODE_AND, "and")
        CASE(OPCODE_OR, "or")
        CASE(OPCODE_XOR, "xor")
        CASE(OPCODE_SHL, "shl")
        CASE(OPCODE_SHR, "shr")
        CASE(OPCODE_JMP, "jmp")
        CASE(OPCODE_JMP1, "jmp1")
        CASE(OPCODE_JMP2, "jmp2")
        CASE(OPCODE_CALL, "call")
        CASE(OPCODE_CALL1, "call1")
        CASE(OPCODE_CALL2, "call2")
        CASE(OPCODE_RET, "ret")
        CASE(OPCODE_SYSCALL, "syscall")
        CASE(OPCODE_VRET, "vret")
        CASE(OPCODE_DBG, "dbg")
        CASE(OPCODE_EINT, "eint")
        CASE(OPCODE_DINT, "dint")
        CASE(OPCODE_SLOW, "slow")
        CASE(OPCODE_WFI, "wfi")
        CASE(OPCODE_NOP, "nop")
        CASE(OPCODE_VMSTART, "vmstart")
        CASE(OPCODE_JZ, "jz")
        CASE(OPCODE_JNZ, "jnz")
        CASE(OPCODE_JCOND, "jcond")
        CASE(OPCODE_LEA, "lea")
        CASE(OPCODE_LDB, "ldb")
        CASE(OPCODE_LDBS, "ldbs")
        CASE(OPCODE_LDH, "ldh")
        CASE(OPCODE_LDHS, "ldhs")
        CASE(OPCODE_LDL, "ldl")
        CASE(OPCODE_LDLS, "ldls")
        CASE(OPCODE_LDQ, "ldq")
        CASE(OPCODE_STB, "stb")
        CASE(OPCODE_STH, "sth")
        CASE(OPCODE_STL, "stl")
        CASE(OPCODE_STQ, "stq")
        CASE(OPCODE_XADD, "xadd")
        CASE(OPCODE_CAS, "cas")
        CASE(OPCODE_SAVE, "save")
        CASE(OPCODE_RESTORE, "restore")
        CASE(OPCODE_RDTICK, "rdtick")
        CASE(OPCODE_RDTICK1, "rdtick1")
        CASE(OPCODE_RDTICK2, "rdtick2")
    }
    Assert(false);
    return "unknown";
}
