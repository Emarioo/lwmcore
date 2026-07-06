#pragma once

#include <stdint.h>


#include "lwm/util.h"
#include "lwm/isa.h"

#include "lwm/parser_util.h"

typedef struct {
    string name;
    int object_id;
    int final_address;
    int estimated_addressLow;
    int estimated_addressHigh;
} Label;

typedef struct {
    string label;
    union {
        int regnum;
        int reg_base;
    };
    int64_t immediate;
    AddressingForm form;
    int reg_index;
} Operand;

typedef struct {
    u8      opcode;
    u8      sub_opcode;
    u8      operands_len;
    int     parseHead;
    Operand operands[4];
} Instruction;

typedef struct {
    u8* bytes;
    int size;
} DataObject;

#define OBJECT_INSTRUCTION 0
#define OBJECT_DATA 1

typedef struct {
    u8 kind;
    u8 alignment; // (1 << alignment)
    union {
        Instruction inst;
        DataObject  dataobj;
    };
} Object;

typedef struct {
    string   name;
    uint64_t addr;
    int      size;
    Location location;
    
    int     objects_len;
    Object* objects;
    
    int     label_len;
    Label*  labels;
} Section;

