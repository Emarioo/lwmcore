#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    // input options
    const char* sourcePath; // only used when printing errors
    // include paths
    // cpu options

    bool quiet;

    // output options
    const char* outputPath;
    void* rom;
    int   rom_len;
    int   rom_max;
} AssemblerOptions;

typedef enum {
    LWM_ASM_ERROR_NONE,
    LWM_ASM_ERROR_BAD,
} AssemblerError;

AssemblerError assemble(const char* text, size_t text_len, AssemblerOptions* options);

