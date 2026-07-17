

#include "lwm/emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//########################
//    Memory Mapped IO
//########################



#define EMU_BASE     0xF000
#define EMU_HALT     (EMU_BASE+0x0)
#define EMU_OUT      (EMU_BASE+0x4)
#define EMU_LOG_BEG  (EMU_BASE+0x8)
#define EMU_LOG_END  (EMU_BASE+0xFFC)


//########################
//    Device Functions
//########################


bool device_write(HardwareDevice* device, uintptr_t address, size_t size, void* data);


//########################
//    Implementation
//########################


typedef struct {
    FILE* log_file;
} EMU_State;

bool device_event(HardwareDevice* device, HardwareEvent eventType, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
    EmulatorContext* emulator = device->emulator;
    EMU_State*       state    = device->state;

    switch (eventType) {
        case EVENT_INIT: {
            state = malloc(sizeof(EMU_State));
            memset(state, 0, sizeof(*state));
            device->state     = state;
            device->eventMask = 0;
            device->mmio_write = device_write;

            declare_mmio(device, EMU_BASE, 4096);

            return true;
        } break;
        default:
    }
    return false;
}

bool device_write(HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    EmulatorContext* emulator = device->emulator;
    EMU_State* state = device->state;

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
