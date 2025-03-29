#pragma once

#include "lwmcore/util.h"

typedef enum {
    // Basic opcode encoding (4 bits)
    INST_LI    = 0b0000,
    INST_JEQ   = 0b0001,
    INST_JNE   = 0b0010,
    INST_JLT   = 0b0011,
    INST_JGT   = 0b0100,
    INST_JMP   = 0b0101,
    INST_CALL  = 0b0110,
    
    // Special opcode encoding (5 bits)
    INST_LODW_SP  = 0b10000,
    INST_LODB_SP  = 0b10001,
    INST_STRW_SP  = 0b10010,
    INST_STRB_SP  = 0b10011,
    
    // Extended opcode encoding (8 bits)
    INST_LODW  = 0b11000000,
    INST_LODB  = 0b11000001,
    INST_STRW  = 0b11000010,
    INST_STRB  = 0b11000011,
    INST_ADD   = 0b11000100,
    INST_SUB   = 0b11000101,
    INST_AND   = 0b11000110,
    INST_OR    = 0b11000111,
    INST_XOR   = 0b11001000,
    INST_CMP   = 0b11001001,
    INST_RET   = 0b11001010,
    INST_SHL   = 0b11001011,
    INST_SHR   = 0b11001100,

    INST_INVALID = -1,
} InstEncoding;

typedef enum {
    // Register encoding (4 bits)
    REG_RA = 0b0000,
    REG_RB = 0b0001,
    REG_RC = 0b0010,
    REG_RD = 0b0011,
    REG_RE = 0b0100,
    REG_RF = 0b0101,
    REG_RG = 0b0110,
    REG_RH = 0b0111,

    REG_SP = 0b1110,
    REG_PC = 0b1111,

    REG_INVALID = -1,
} RegEncoding;

int get_regnr(string name);
InstEncoding get_opcode(string name);
string get_opcode_name(int opcode);

extern const char* inst_name_table[256];