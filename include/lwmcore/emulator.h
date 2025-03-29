#pragma once

#include "lwmcore/util.h"
#include "lwmcore/encoding.h"

typedef struct {
    u16 registers[16];

    int stack_max;
    char* stack;
} CPU;

typedef struct {
    int error_code;

    CPU cpu;
} EmulatorInfo;

//#######################
//   PUBLIC FUNCTIONS
//#######################
bool emulate(Bin bin, EmulatorInfo* info);

void dump(Bin bin);

//#######################
//   PRIVATE FUNCTIONS
//#######################

