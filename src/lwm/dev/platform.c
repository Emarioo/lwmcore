

#include "lwm/emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//########################
//    Memory Mapped IO
//########################


#define PLATFORM_BASE            0xEFE0
#define PLATFORM_CORE_CONTROL    PLATFORM_BASE+0x0
#define PLATFORM_CORE_CPUID      PLATFORM_BASE+0x2
#define PLATFORM_CORE_ENTRY      PLATFORM_BASE+0x8

#define PLATFORM_CORE_CONTROL_BOOT  0x1
#define PLATFORM_CORE_CONTROL_RESET 0x2

//########################
//    Device Functions
//########################


bool platform_init(EmulatorContext* emulator, HardwareDevice* device);
bool platform_mmio_write(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);
bool platform_mmio_read(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);
void platform_tick(EmulatorContext* emulator, HardwareDevice* device);


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


bool platform_init(EmulatorContext* emulator, HardwareDevice* device) {
    Platform_State* state = malloc(sizeof(*state));
    memset(state, 0, sizeof(*state));

    device->state           = state;
    device->mmio_write      = platform_mmio_write;
    // device->mmio_read       = platform_mmio_read;
    // device->tick            = platform_tick;
    return true;
}


void platform_tick(EmulatorContext* emulator, HardwareDevice* device) {
    Platform_State* state = device->state;

}

bool platform_mmio_write(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    Platform_State* state = device->state;
    // printf("PLATFORM MMIO 0x%zx %zd %c\n", address, size, *(char*)data);
    
    if (size < 1 || size > 8)
        return false;
    
    if (address >= PLATFORM_BASE && address + size <= PLATFORM_BASE + sizeof(state->ram)) {
        // printf("IC MMIO 0x%zx %zd %d\n", address, size, *(char*)data);
        memcpy((char*)&state->ram + address - PLATFORM_BASE, data, size);

        if (address == PLATFORM_CORE_CONTROL) {
            if (state->ram.core_control & PLATFORM_CORE_CONTROL_BOOT) {
                printf("Boot core%d at 0x%zx\n", state->ram.core_cpuid, state->ram.core_entry);
                emulator_boot_core(emulator, state->ram.core_cpuid, state->ram.core_entry);
            }
            if (state->ram.core_control & PLATFORM_CORE_CONTROL_RESET) {
                printf("Reset core%d\n", state->ram.core_cpuid);
                emulator_reset_core(emulator, state->ram.core_cpuid);
            }
        }
        return true;
    }
    return false;
}


bool platform_mmio_read(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    Platform_State* state = device->state;
    // printf("IC MMIO 0x%zx %zd %c\n", address, size, *(char*)data);
    
    // if (size < 1 || size > 4)
    //     return false;

    // if (address >= IC_BASE && address + size <= IC_BASE + sizeof(IC_RAM)) {
    //     memcpy(data, (char*)&state->ic_ram + address - IC_BASE, size);
    //     return true;
    // }

    return false;
}
