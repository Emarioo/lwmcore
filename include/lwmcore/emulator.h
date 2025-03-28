#pragma once

#include "lwmcore/util.h"

typedef struct {
    int error_code;
} EmulatorInfo;

bool emulate(Bin bin, EmulatorInfo* info);