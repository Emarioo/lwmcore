#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // include paths
    // cpu options

    // output options
} AssemblerOptions;

typedef enum {
    LWM_ASM_ERROR_NONE,
} AssemblerError;

AssemblerError assemble(const char* text, size_t text_len, AssemblerOptions* options);
