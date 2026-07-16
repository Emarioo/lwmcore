

#include "lwm/emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//########################
//    Memory Mapped IO
//########################


#define EMU_LOG_BEG  0xF008
#define EMU_LOG_END  0xFFFC

#define EMU_HALT     0xF000
#define EMU_OUT      0xF004


//########################
//    Device Functions
//########################


bool device_init(EmulatorContext* emulator, HardwareDevice* device);
bool emu_mmio_write(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);


//########################
//    Implementation
//########################


typedef struct {
    FILE* log_file;
} EMU_State;


bool device_init(EmulatorContext* emulator, HardwareDevice* device) {
    EMU_State* state = malloc(sizeof(EMU_State));
    memset(state, 0, sizeof(*state));
    device->mmio_write = emu_mmio_write;
    device->state      = state;
    return true;
}



bool emu_mmio_write(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    EMU_State* state = device->state;

    // printf("MMIO 0x%zx\n", address);

    if (address == EMU_HALT) {
        // Memory mapped IO for halting the emulator.
        if (emulator->platformConfig->verbose) {
            printf("MMIO HALT\n");
        }
        for (int i=0;i<emulator->platformConfig->core_count;i++) {
            emulator_reset_core(emulator, i);
        }
        return true;
    }
    if (size == 1 && address == EMU_OUT) {
        printf("%c", *(char*)data);
        fflush(stdout);
        return true;
    }
    if (size == 1 && address >= EMU_LOG_BEG && address < EMU_LOG_END) {
        if (!state->log_file) {
            state->log_file = fopen("emu.log", "w");
        }
        fwrite(data, 1, 1, state->log_file);
        fflush(state->log_file);
        return true;
    }
    return false;
}
