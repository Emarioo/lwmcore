#pragma once

#include "lwmcore/util.h"

typedef struct {
    int error_code;
} AssemblerInfo;

bool assemble(string text, Bin* bin, AssemblerInfo* opt_info);