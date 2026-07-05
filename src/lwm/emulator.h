

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


typedef enum {
    MODE_16,
    MODE_32,
    MODE_64,
} CoreMode;


typedef struct {

    CoreMode mode;

    uint64_t gprs[32];
    uint64_t pc;
    #define tp gprs[13]
    #define lr gprs[14]
    #define sp gprs[15]

    uint64_t crs[32];

    #define crstatus crs[0]
    #define crvb     crs[1]
    #define crpt     crs[2]
    #define crepc    crs[3]
    #define crcause  crs[4]
    #define crfault  crs[5]
    #define crcpuid  crs[6]
    #define crtimercmp  crs[7]

    uint64_t tickCounter;

} CoreState;

typedef bool(*FN_mmio_read )(uintptr_t address, size_t size, void* data);
typedef bool(*FN_mmio_write)(uintptr_t address, size_t size, void* data);

typedef struct {
    FN_mmio_read  mmio_read;
    FN_mmio_write mmio_write;
} HardwareDevice;

typedef struct {
    uint64_t core_entry;
    void*    rom;
    int      rom_len;

    int              devices_len;
    HardwareDevice** devices;
} PlatformConfig;



void emulator_start(PlatformConfig* config);

void dump(PlatformConfig* config);
