#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <setjmp.h>

#include "lwm/isa.h"

typedef struct EmulatorContext EmulatorContext;


typedef enum {
    MODE_16,
    MODE_32,
    MODE_64,
} CoreMode;


typedef struct {

    CoreMode mode;

    union {
        uint64_t gprs[32];
        struct {
            uint64_t _[13];
            uint64_t tp;
            uint64_t lr;
            uint64_t sp;
        };
    };
    uint64_t pc;

    union {
        uint64_t crs[32];
        struct {
            uint64_t crstatus;
            uint64_t crvb;        // Vector base
            uint64_t crpt;        // Root page table
            uint64_t crisp;       // interrupt stack pointer. SP = ISP on exceptions

            uint64_t crestatus;   // ESTATUS = STATUS on exceptions
            uint64_t crepc;       // EPC = PC on exceptions
            uint64_t cresp;       // ESP = SP on exceptions
            uint64_t crcause;
            uint64_t crfault;

            uint64_t crcpuid;
            uint64_t crtimercmp;
            uint64_t crkernel;    // Reserved control register for Kernel
        };
    };

    // bool insideInterrupt;
    bool insideFault;
    bool insideDoubleFault;
    
    // First 32 bits are unused because interrupt controller is not
    // allowed to send interrupts to exception handler vectors like page fault.
    // Ignored if you do i guess?
    uint32_t pendingVectors[MAX_VECTORS/32];

    uint64_t tickCounter;
    uint64_t avgTickFrequency;
    uint64_t instructionSteps;
    uint64_t executionTime_ns;

    bool running;
    jmp_buf loop_jmpbuf;

    EmulatorContext* emulator;
} CoreState;


#define BITS_PER_PENDING_VECTOR_INDEX(CORE) (8*sizeof(*(CORE)->pendingVectors))
#define GET_PENDING_VECTOR(CORE, VECTOR) \
    (((CORE)->pendingVectors[VECTOR/BITS_PER_PENDING_VECTOR_INDEX(CORE)] >> (VECTOR%BITS_PER_PENDING_VECTOR_INDEX(CORE))) & 1)
#define ENABLE_PENDING_VECTOR(CORE, VECTOR) \
    ((CORE)->pendingVectors[VECTOR/BITS_PER_PENDING_VECTOR_INDEX(CORE)] |= (uint64_t)1 << (VECTOR%BITS_PER_PENDING_VECTOR_INDEX(CORE)))
#define DISABLE_PENDING_VECTOR(CORE, VECTOR) \
    ((CORE)->pendingVectors[VECTOR/BITS_PER_PENDING_VECTOR_INDEX(CORE)] &= ~((uint64_t)1 << (VECTOR%BITS_PER_PENDING_VECTOR_INDEX(CORE))))


typedef struct HardwareDevice HardwareDevice;

#define DEVICE_FUNC_INIT  "device_init"

typedef bool(*FN_mmio_read)      (EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);
typedef bool(*FN_mmio_write)     (EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);
typedef void(*FN_tick)           (EmulatorContext* emulator, HardwareDevice* device);
typedef bool(*FN_init)           (EmulatorContext* emulator, HardwareDevice* device);
typedef void(*FN_queue_interrupt)(EmulatorContext* emulator, HardwareDevice* device, int irq_number);

struct HardwareDevice {
    const char*        sharedLibraryPath;
    void*              sharedLibrary;
    void*              state;
    void*              user_data;
    FN_init            init;
    // @TODO deinit so we can join threads that were created.
    //    Needed for display device which creates a raylib thread which
    //    segfaults at the end because we don't clean it up (I assume that's why we segfault)
    FN_tick            tick;
    FN_mmio_read       mmio_read;
    FN_mmio_write      mmio_write;
    FN_queue_interrupt queue_interrupt; // Only for interrupt controllers
};

typedef struct {
    const char* rom_path;
    void*       rom;
    int         rom_len;
    
    uint64_t core_entry;
    uint64_t ram_size;
    int      core_count;
    CoreMode core_mode;

    int              devicePaths_len;
    const char**     devicePaths;

    int              devices_len;
    HardwareDevice** devices;

    bool quiet;
    bool verbose;
} PlatformConfig;

struct EmulatorContext {
    PlatformConfig* platformConfig;

    uint8_t* physicalMemory;
    uint64_t physicalMemory_size;

    uint32_t randomState;

    CoreState* cores;
};


void emulator_request_interrupt(EmulatorContext* emulator, int irq_number);
void emulator_raise_vector(EmulatorContext* emulator, int cpuid, int vector);

void emulator_boot_core(EmulatorContext* emulator, int cpuid, uintptr_t entry);
void emulator_reset_core(EmulatorContext* emulator, int cpuid);
