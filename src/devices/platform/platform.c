

#include "lwm/emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//########################
//    Memory Mapped IO
//########################


#define PLATFORM_BASE            0xEFC0
#define PLATFORM_CORE_CONTROL    (PLATFORM_BASE+0x0)
#define PLATFORM_CORE_CPUID      (PLATFORM_BASE+0x2)
#define PLATFORM_CORE_ENTRY      (PLATFORM_BASE+0x8)
#define PLATFORM_CORE_TICK_FREQ  (PLATFORM_BASE+0x10)

#define PLATFORM_CORE_CONTROL_BOOT  0x1
#define PLATFORM_CORE_CONTROL_RESET 0x2

//########################
//    Device Functions
//########################

bool device_read(HardwareDevice* device, uintptr_t address, size_t size, void* data);
bool device_write(HardwareDevice* device, uintptr_t address, size_t size, void* data);

//########################
//    Implementation
//########################


typedef struct {
    uint16_t  core_control;
    uint16_t  core_cpuid;
    uint16_t  core_padding[2];
    uintptr_t core_entry;
} Platform_RAM;


typedef struct {
    Platform_RAM ram;
} Platform_State;


bool device_event(HardwareDevice* device, HardwareEvent eventType, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
    EmulatorContext* emulator = device->emulator;
    Platform_State*  state    = device->state;

    switch (eventType) {
        case EVENT_INIT: {
            state = malloc(sizeof(*state));
            memset(state, 0, sizeof(*state));
            device->state     = state;
            device->eventMask = 0;
            device->mmio_read = device_read;
            device->mmio_write = device_write;

            declare_mmio(device, PLATFORM_BASE, 0x20);

            return true;
        } break;
        default:
    }
    return false;
}
bool device_read(HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    EmulatorContext*  emulator = device->emulator;
    Platform_State*  state    = device->state;
    
    if (size < 1 || size > 8)
    return false;
    
    if (address >= PLATFORM_CORE_TICK_FREQ && address <= PLATFORM_CORE_TICK_FREQ + 8 - size) {
        memcpy(data, (char*)&emulator->cores->avgTickFrequency + address - PLATFORM_CORE_TICK_FREQ, size);
        return true;
    }
    
    return false;
}
bool device_write(HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    EmulatorContext*  emulator = device->emulator;
    Platform_State*  state     = device->state;
    
    if (size < 1 || size > 8)
        return false;
    
    if (address >= PLATFORM_BASE && address <= PLATFORM_BASE + sizeof(state->ram) - size) {
        // printf("IC MMIO 0x%zx %zd %d\n", address, size, *(char*)data);
        memcpy((char*)&state->ram + address - PLATFORM_BASE, data, size);

        if (address == PLATFORM_CORE_CONTROL) {
            if (state->ram.core_control & PLATFORM_CORE_CONTROL_BOOT) {
                if (emulator->platformConfig->verbose) {
                    printf("Boot core%d at 0x%zx\n", state->ram.core_cpuid, state->ram.core_entry);
                }
                emulator_boot_core(emulator, state->ram.core_cpuid, state->ram.core_entry);
            }
            if (state->ram.core_control & PLATFORM_CORE_CONTROL_RESET) {
                if (emulator->platformConfig->verbose) {
                    printf("Reset core%d\n", state->ram.core_cpuid);
                }
                emulator_reset_core(emulator, state->ram.core_cpuid);
            }
        }
        return true;
    }
    return false;
}
