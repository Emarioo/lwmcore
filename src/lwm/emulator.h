

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

    bool running;
    jmp_buf loop_jmpbuf;

    EmulatorContext* emulator;
} CoreState;

typedef struct HardwareDevice HardwareDevice;

typedef bool(*FN_mmio_read)      (EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);
typedef bool(*FN_mmio_write)     (EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);
typedef void(*FN_tick)           (EmulatorContext* emulator, HardwareDevice* device);
typedef bool(*FN_init)           (EmulatorContext* emulator, HardwareDevice* device);
typedef void(*FN_queue_interrupt)(EmulatorContext* emulator, HardwareDevice* device, int irq_number);

struct HardwareDevice {
    void*              state;
    void*              user_data;
    FN_init            init;
    FN_tick            tick;
    FN_mmio_read       mmio_read;
    FN_mmio_write      mmio_write;
    FN_queue_interrupt queue_interrupt; // Only for interrupt controllers
};

typedef struct {
    uint64_t core_entry;
    void*    rom;
    int      rom_len;

    int      core_count;
    CoreMode core_mode;

    bool quiet;
    bool verbose;

    int              devices_len;
    HardwareDevice** devices;
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

void emulator_start(PlatformConfig* config);

void dump(PlatformConfig* config);
