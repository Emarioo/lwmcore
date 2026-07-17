#pragma once

#include <stdint.h>


#include "lwm/util.h"
#include "lwm/isa.h"

#include "lwm/parser_util.h"

typedef struct {
    string name;
    int object_id;
    uintptr_t final_address;
    uintptr_t estimated_addressLow;
    uintptr_t estimated_addressHigh;
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
    uint32_t offset; // relative to data object
    string   label;
} LabelValue;

typedef struct {
    u8*         bytes;
    int         size;
    uint16_t    elementSize;
    uint16_t    labels_len;
    LabelValue* labels;
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
    uint64_t size;
    SourceLocation location;
    
    int     objects_len;
    int     objects_max;
    Object* objects;
    
    int     labels_len;
    int     labels_max;
    Label*  labels;

    uint64_t estimatedAddress_low; // relative to section
    uint64_t estimatedAddress_high;
} Section;

