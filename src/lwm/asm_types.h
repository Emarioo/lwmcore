#pragma once

#include <stdint.h>

#define MAX_LABELS_PER_BLOCK 100
#define MAX_INST_PER_BLOCK 100
#define MAX_BLOCKS 100
#define MAX_LOCATION_MAPPING 100

#include "lwm/util.h"
#include "lwm/lwm_isa.h"

// typedef struct {
//     char*    ptr;
//     uint32_t len;
//     uint32_t max;
// } string;

typedef struct {
    const char* file;
    int line;
    int column;
} Location;

typedef struct {
    string name;
    int addr; // relative to block
} Label;

typedef struct {
    int regnum;
    int64_t immediate;
    string label;

    AddressingForm form;
    int reg_base;
    int reg_index;
    // @TODO Addressing mode.
} Operand;

typedef struct {
    u8      opcode;
    u8      sub_opcode;
    u8      operands_len;
    Operand operands[4];
} Instruction;


typedef struct {
    uint64_t addr;
    int size;
    Location location;
    
    int inst_len;
    Instruction insts[MAX_INST_PER_BLOCK]; // TODO: Increase limit
    
    int label_len;
    Label labels[MAX_LABELS_PER_BLOCK]; // TODO: Increase limit
} Block;


typedef struct {
    int head_start;
    int head_end;
    string file;
    int line;
} LocationMapping;


typedef struct {
    string source_file;
    int error_code;

    char* text;
    int   text_len;

    
    int block_len;
    Block blocks[MAX_BLOCKS]; // TODO: Increase limit

    int loc_map_len;
    LocationMapping loc_map[MAX_LOCATION_MAPPING]; // TODO: Increase limit

    jmp_buf jumpBuffer;
} AssemblerContext;


