#pragma once

#include <stdint.h>

#include "lwm/util.h"


#define FIELD_NAME_MAX 16
#define PRIORITY_NAME_MAX 16
#define INSTRUCTION_NAME_MAX 32

typedef struct {
    char name[FIELD_NAME_MAX];
    int bits;
} scheme_Field;

typedef struct {
    char name[PRIORITY_NAME_MAX];
    int priority;
} scheme_Priority;

typedef struct {
    // Sum all these bits to get length of instruction
    uint8_t numOpcodeBits;
    uint8_t numUnusedBits;
    uint8_t numNonOpcodeBits; // fixed per instruction
    uint8_t fixedBits[20];
} scheme_Encoding;

typedef struct {
    char name[INSTRUCTION_NAME_MAX];

    int              fields_len;
    scheme_Field*    fields[8];
    scheme_Priority* priority;

    scheme_Encoding* encoding;

} scheme_Instruction;

typedef struct {
    int                 fields_len;
    scheme_Field*       fields;
    int                 priorities_len;
    scheme_Priority*    priorities;
    int                 instructions_len;
    scheme_Instruction* instructions;
} scheme_Database;




void parse_scheme(const char* path, scheme_Database* database);

void generate_encoder(scheme_Database* database);

void dump_scheme(scheme_Database* db);

scheme_Field*       find_field(scheme_Database* db, const char* name);
scheme_Priority*    find_priority(scheme_Database* db, const char* name);
scheme_Instruction* find_instruction(scheme_Database* db, const char* name);
