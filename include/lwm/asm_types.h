#pragma once

#include <stdint.h>


#include "lwm/util.h"
#include "lwm/isa.h"
#include "lwm/encoding.h"

#include "lwm/parser_util.h"

typedef struct Label Label;
struct Label {
    string name;
    int object_id;
    uintptr_t final_address;
    uintptr_t estimated_addressLow;
    uintptr_t estimated_addressHigh;
};


typedef struct LabelValue LabelValue;
struct LabelValue {
    uint32_t offset; // relative to data object
    string   label;
};

typedef struct DataObject DataObject;
struct DataObject {
    u8*         bytes;
    int         size;
    uint16_t    elementSize;
    uint16_t    labels_len;
    LabelValue* labels;
};

#define OBJECT_INSTRUCTION 0
#define OBJECT_DATA 1

typedef struct Object Object;
struct Object {
    u8 kind;
    u8 alignment; // (1 << alignment)
    union {
        Instruction inst;
        DataObject  dataobj;
    };
};

typedef struct Section Section;
struct Section {
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
};


typedef struct LabelFixup LabelFixup;
struct LabelFixup {
    Label*   label;
    uint64_t rom_offset;
    uint8_t  reloc_size;
    bool     absolute;
};

