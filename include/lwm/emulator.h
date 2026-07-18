#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <setjmp.h>

#include "lwm/isa.h"
#include "lwm/encoding.h"

typedef struct EmulatorContext EmulatorContext;


typedef enum {
    MODE_16,
    MODE_32,
    MODE_64,
} CoreMode;

typedef struct CoreState CoreState;
struct CoreState {

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
    uint64_t pendingVectors[MAX_VECTORS/64];

    uint64_t tickCounter;
    uint64_t avgTickFrequency;
    uint64_t instructionSteps;
    uint64_t executionTime_ns;

    bool running;
    jmp_buf loop_jmpbuf;

    Decoder decoder;
    EmulatorContext* emulator;
};


#define BITS_PER_PENDING_VECTOR_INDEX(CORE) (8*sizeof(*(CORE)->pendingVectors))
#define ANY_PENDING_VECTOR(CORE) \
    ((CORE)->pendingVectors[0])
#define GET_PENDING_VECTOR(CORE, VECTOR) \
    (((CORE)->pendingVectors[VECTOR/BITS_PER_PENDING_VECTOR_INDEX(CORE)] >> (VECTOR%BITS_PER_PENDING_VECTOR_INDEX(CORE))) & 1)
#define ENABLE_PENDING_VECTOR(CORE, VECTOR) \
    ((CORE)->pendingVectors[VECTOR/BITS_PER_PENDING_VECTOR_INDEX(CORE)] |= (uint64_t)1 << (VECTOR%BITS_PER_PENDING_VECTOR_INDEX(CORE)))
#define DISABLE_PENDING_VECTOR(CORE, VECTOR) \
    ((CORE)->pendingVectors[VECTOR/BITS_PER_PENDING_VECTOR_INDEX(CORE)] &= ~((uint64_t)1 << (VECTOR%BITS_PER_PENDING_VECTOR_INDEX(CORE))))


typedef struct HardwareDevice HardwareDevice;

#define FUNC_DEVICE_EVENT "device_event"

typedef enum {
    EVENT_INIT        = 0x1,
    EVENT_DEINIT      = 0x2,
    EVENT_INTERRUPT   = 0x4, // Only used by interrupt controller
} HardwareEvent;

typedef bool(*FN_event)(HardwareDevice* device, HardwareEvent eventType, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3);
typedef bool(*FN_mmio)(HardwareDevice* device, uintptr_t address, size_t size, void* data);
typedef bool(*FN_tick)(HardwareDevice* device);

struct HardwareDevice {
    EmulatorContext*   emulator;
    const char*        sharedLibraryPath;
    void*              sharedLibrary;
    void*              state;
    void*              user_data;
    FN_event           event;
    HardwareEvent      eventMask;
    FN_mmio            mmio_read;
    FN_mmio            mmio_write;
    FN_tick            tick;

    uintptr_t          mmio_start;
    uintptr_t          mmio_end;
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

    uintptr_t mmio_start;
    uintptr_t mmio_end;

    int              mmioReadDevices_len;
    HardwareDevice** mmioReadDevices;
    int              mmioWriteDevices_len;
    HardwareDevice** mmioWriteDevices;
    int              tickDevices_len;
    HardwareDevice** tickDevices;

    uint32_t randomState;

    CoreState* cores;
};


void emulator_request_interrupt(EmulatorContext* emulator, int irq_number);
void emulator_raise_vector(EmulatorContext* emulator, int cpuid, int vector);

void emulator_boot_core(EmulatorContext* emulator, int cpuid, uintptr_t entry);
void emulator_reset_core(EmulatorContext* emulator, int cpuid);

void declare_mmio(HardwareDevice* device, uintptr_t address, size_t size);

// Defined in the device code
bool device_event(HardwareDevice* device, HardwareEvent eventType, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3);
