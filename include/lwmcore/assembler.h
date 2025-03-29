#pragma once

#include "lwmcore/util.h"

#define MAX_LABELS_PER_BLOCK 100
#define MAX_INST_PER_BLOCK 100
#define MAX_BLOCKS 100
#define MAX_LOCATION_MAPPING 100

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
    Instruction insts[MAX_LABELS_PER_BLOCK]; // TODO: Increase limit
    
    int label_len;
    Label labels[MAX_INST_PER_BLOCK]; // TODO: Increase limit
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
    
    int block_len;
    Block blocks[MAX_BLOCKS]; // TODO: Increase limit

    int loc_map_len;
    LocationMapping loc_map[MAX_LOCATION_MAPPING]; // TODO: Increase limit
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
Location count_lines(string* text, int head, AssemblerInfo* info);