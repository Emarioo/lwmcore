

#include "lwm/emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//########################
//    Memory Mapped IO
//########################


#define IC_BASE               0xE000
#define IC_EOI                IC_BASE+0x0
#define IC_IRQ_BASE           IC_BASE+0x4
#define IC_IRQ_OFFSET_VECTOR  0x0
#define IC_IRQ_OFFSET_CPU     0x1
#define IC_IRQ_OFFSET_FLAGS   0x2
#define IC_IRQ_STRIDE         4

#define IC_FLAGS_ENABLE_MASK  0x1
#define IC_FLAGS_ENABLE_BIT   0

#define IC_MAX_IRQS           64


//########################
//    Device Functions
//########################


bool dev_create_ic(EmulatorContext* emulator, HardwareDevice* device);
bool ic_mmio_write(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);
bool ic_mmio_read(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);
void ic_tick(EmulatorContext* emulator, HardwareDevice* device);
void ic_queue_interrupt(EmulatorContext* emulator, HardwareDevice* device, int irq_number);

//########################
//    Implementation
//########################


typedef struct {
    uint8_t  vector;
    uint8_t  cpu;
    uint16_t flags;
} IRQEntry;

typedef struct {
    uint32_t eoi;
    IRQEntry entries[IC_MAX_IRQS];
} IC_RAM;


typedef struct {
    IC_RAM ic_ram;
    int    inService;
    bool   pendingIRQs[IC_MAX_IRQS];
} IC_State;


bool dev_create_ic(EmulatorContext* emulator, HardwareDevice* device) {
    IC_State* state = malloc(sizeof(IC_State));
    memset(state, 0, sizeof(*state));

    device->state           = state;
    device->mmio_write      = ic_mmio_write;
    device->mmio_read       = ic_mmio_read;
    device->tick            = ic_tick;
    device->queue_interrupt = ic_queue_interrupt;
    return true;
}

void ic_queue_interrupt(EmulatorContext* emulator, HardwareDevice* device, int irq_number) {
    IC_State* state = device->state;
    // if (!state->pendingIRQs[irq_number])
    // printf("IRQ %d\n", irq_number);
    state->pendingIRQs[irq_number] = true;
}

void ic_tick(EmulatorContext* emulator, HardwareDevice* device) {
    IC_State* state = device->state;

    bool* interruptLine = (bool*)&emulator->coreState.interruptLine;
    int* vectorIndex    = (int*)&emulator->coreState.vectorIndex;

    if (!state->inService) {
        *interruptLine = false;
    }
    
    // for (int i=0;i<IC_MAX_IRQS;i++) {
    for (int i=0;i<2;i++) {
        IRQEntry* entry = &state->ic_ram.entries[i];
        // printf("irq[%d] %d %d %d\n", i, entry->vector, entry->cpu, entry->flags);
        if ((entry->flags & IC_FLAGS_ENABLE_MASK) == 0) {
            state->pendingIRQs[i] = false;
            continue;
        }

        if (state->pendingIRQs[i] && !state->inService) {
            state->inService = true;
            state->pendingIRQs[i] = false;

            // @TODO Multiple cores!?
            *interruptLine = true;
            *vectorIndex = entry->vector;
            state->ic_ram.eoi = 1;

            // printf("INT LINE %d\n", *vectorIndex);
        }
    }
}

bool ic_mmio_write(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    IC_State* state = device->state;
    // printf("IC MMIO 0x%zx %zd %c\n", address, size, *(char*)data);
    
    if (size < 1 || size > 4)
        return false;

    if (address == IC_EOI) {
        memcpy(&state->ic_ram.eoi, data, size);
        if (*(char*)data == 0) {
            state->inService = false;
        }
        return true;
    }
    
    if (address >= IC_BASE && address + size <= IC_BASE + sizeof(IC_RAM)) {
        // printf("IC MMIO 0x%zx %zd %d\n", address, size, *(char*)data);
        memcpy((char*)&state->ic_ram + address - IC_BASE, data, size);
        // @TODO Update IRQs and interrupts
        return true;
    }

    return false;
}


bool ic_mmio_read(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    IC_State* state = device->state;
    // printf("IC MMIO 0x%zx %zd %c\n", address, size, *(char*)data);
    
    if (size < 1 || size > 4)
        return false;

    if (address >= IC_BASE && address + size <= IC_BASE + sizeof(IC_RAM)) {
        memcpy(data, (char*)&state->ic_ram + address - IC_BASE, size);
        return true;
    }

    return false;
}
