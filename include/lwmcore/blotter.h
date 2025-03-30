/*
    1. Create logic world save files (and subassemblies)
    2. Read and modify existing files?

    Structures are based on Blotter v7 (file format for Logic World)
*/

#pragma once

#include "lwmcore/util.h"

#pragma pack(push, 1)
typedef struct {
    char magic[16];
    // Save info
    i8 save_format_version;
    int game_version[4];
    i8 save_type;
    i32 num_components;
    i32 num_wires;
    // mod versions
    // component ids map
    // Object data
} BlotterHeader;
#pragma pack(pop)

typedef struct {
    char magic[16];
} BlotterFooter;

typedef enum {
    LW_SAVE_TYPE_UNKNOWN,
    LW_SAVE_TYPE_WORLD = 1,
    LW_SAVE_TYPE_SUBASSEMBLY = 2,
} BlotterSaveType;

#pragma pack(push, 1)
typedef struct {
    int addr;
    int parent_addr;
    i16 comp_id;
    int position[3];
    float rotation[4];
} BlotterComponentHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    i8 type;
    i32 addr;
    i32 index;
} BlotterPegAddress;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    BlotterPegAddress first;
    BlotterPegAddress second;
    i32 circuit_state;
    float rotation;
} BlotterWire;
#pragma pack(pop)

// Custom data for components
// (can't find actual format of custom data anywhere, I have guessed these formats)
#pragma pack(push, 1)
typedef struct {
    u8 red;
    u8 green;
    u8 blue;
    int width;
    int height;
} BlotterCustomData_CircuitBoard;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    int _0; // not sure
    u8 red;
    u8 green;
    u8 blue;
    u8 _1[5]; // not sure
    // variable length string (4 bytes for length + char bytes)
    // int _2[3]; // width/height/depth?
} BlotterCustomData_PanelLabel;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    u8 red;
    u8 green;
    u8 blue;
    u8 active;
} BlotterCustomData_Button;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    u8 red;
    u8 green;
    u8 blue;
    u8 active;
} BlotterCustomData_Switch;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    u8 _0[4]; // not sure
    int _1; // not sure
} BlotterCustomData_Delayer;
#pragma pack(pop)

typedef struct {
    int x;
} BlotterData;

//###########################
//     PUBLIC FUNCTIONS
//###########################

BlotterData* decompose_blotter_file(string path);
// source_path is used in error message
BlotterData* decompose_blotter_data(string data, const char* source_path);

// write blotter logic world file data
void write_logicworld();
// create a folder with subassembly metadata and blotter logic world data
void write_subassembly();

// outputs rom_data
bool modify_rom_subassembly(string data, string* rom_data);

bool write_metadata(string folder_path, const char* title, bool force);

//###########################
//     PRIVATE FUNCTIONS
//###########################

string decompose_blotter_string(string* data, int* head);