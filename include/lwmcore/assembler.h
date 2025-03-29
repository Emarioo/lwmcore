#pragma once

#include "lwmcore/util.h"

#define MAX_LABELS 100
#define MAX_BLOCKS 100
#define MAX_INST_PER_BLOCK 100

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
    string name;
    int reg0;
    int reg1;
    int imm;
    string goto_label;
} Instruction;

typedef struct {
    int addr;
    int size;
    Location location;
    
    int inst_len;
    Instruction insts[100]; // TODO: Increase limit
    
    int label_len;
    Label labels[100]; // TODO: Increase limit
} Block;

typedef struct {
    string source_file;
    int error_code;
    
    int block_len;
    Block blocks[100]; // TODO: Increase limit
} AssemblerInfo;

//#######################
//   PUBLIC FUNCTIONS
//#######################
bool assemble(string text, Bin* bin, AssemblerInfo* info);


//#######################
//   PRIVATE FUNCTIONS
//#######################
int parse_name(string* text, int* head, string* name);
int parse_int(string* text, int* head, int* value);
int parse_space(string* text, int* head);
int parse_string(string* text, int* head, string* str);
int skip_line(string* text, int* head);
int get_regnr(string name);
Location count_lines(string* text, int head, const char* f);