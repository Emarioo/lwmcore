
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
    switch (reg) {
        case LWM_REGNR_CRSTATUS:   sprintf(regname, "CRSTATUS"); break;
        case LWM_REGNR_CRVB:       sprintf(regname, "CRVB"); break;
        case LWM_REGNR_CRPT:       sprintf(regname, "CRPT"); break;
        case LWM_REGNR_CREPC:      sprintf(regname, "CREPC"); break;
        case LWM_REGNR_CRCAUSE:    sprintf(regname, "CRCAUSE"); break;
        case LWM_REGNR_CRFAULT:    sprintf(regname, "CRFAULT"); break;
        case LWM_REGNR_CRCPUID:    sprintf(regname, "CRCPUID"); break;
        case LWM_REGNR_CRTIMERCMP: sprintf(regname, "CRTIMERCMP"); break;
        default:                   sprintf(regname, "cr%d", reg); break;
    }
}

int largest_encoding(int opcode, AddressingForm form) {

    switch ((OpcodeKind)opcode) {
        case OPCODE_LI8:        return 3;
        case OPCODE_LI16:       return 4;
        case OPCODE_LI32:       return 6;
        case OPCODE_LI64:       return 10;
        case OPCODE_CALL_REG:
        case OPCODE_JMP_REG:
        case OPCODE_PUSH:
        case OPCODE_POP:
        case OPCODE_TLBFLUSH:
            return 2;
        case OPCODE_NOT:
        case OPCODE_MFCR:
        case OPCODE_MTCR:
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
            switch (form) {
                case ADDRESSING_ABS16:          return 4;
                case ADDRESSING_ABS32:          return 6;
                case ADDRESSING_ABS64:          return 10;
                case ADDRESSING_REG1_DISP8:     return 3;
                case ADDRESSING_REG1_DISP16:    return 4;
                case ADDRESSING_REG1_DISP32:    return 6;
                case ADDRESSING_REG2_DISP8:     return 4;
                case ADDRESSING_REG2_DISP16:    return 5;
                case ADDRESSING_REG2_DISP32:    return 7;
                case ADDRESSING_PC_DISP32:      return 6;
            }
        case OPCODE_RDTICK: return 2;
        case OPCODE_RDTICK1: return 3;
        case OPCODE_RDTICK2: return 5;
    }
    Assert(false);
    return -1;
}


