

#include "lwm/emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//########################
//    Memory Mapped IO
//########################


#define IC_BASE               0xE000
#define IC_CONTROL            IC_BASE+0x0
#define IC_IPI_CPUID          IC_BASE+0x4
#define IC_IPI_VECTOR         IC_BASE+0x6
#define IC_IRQ_BASE           IC_BASE+0x10
#define IC_IRQ_OFFSET_VECTOR  0x0
#define IC_IRQ_OFFSET_CPU     0x1
#define IC_IRQ_OFFSET_FLAGS   0x2
#define IC_IRQ_STRIDE         4

#define IC_CONTROL_IPI_SEND_MASK  0x1

#define IC_FLAGS_ENABLE_MASK  0x1
#define IC_FLAGS_ENABLE_BIT   0

#define IC_MAX_IRQS           64


//########################
//    Device Functions
//########################

bool device_write(HardwareDevice* device, uintptr_t address, size_t size, void* data);
bool device_read(HardwareDevice* device, uintptr_t address, size_t size, void* data);

//########################
//    Implementation
//########################


typedef struct {
    uint8_t  vector;
    uint8_t  cpuid;
    uint16_t flags;
} IRQEntry;

typedef struct {
    uint32_t control;
    uint16_t ipi_cpuid;
    uint16_t ipi_vector;
    uint32_t eoi;
    uint32_t padding1;
    IRQEntry entries[IC_MAX_IRQS];
} IC_RAM;


typedef struct {
    IC_RAM ic_ram;
} IC_State;


bool device_event(HardwareDevice* device, HardwareEvent eventType, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
    EmulatorContext* emulator = device->emulator;
    IC_State*        state    = device->state;

    switch (eventType) {
        case EVENT_INIT: {
            state = malloc(sizeof(*state));
            memset(state, 0, sizeof(*state));
            device->state     = state;
            device->eventMask = EVENT_INTERRUPT;
            device->mmio_read = device_read;
            device->mmio_write = device_write;

            declare_mmio(device, IC_BASE, 0x10);
            declare_mmio(device, IC_IRQ_BASE, IC_MAX_IRQS * IC_IRQ_STRIDE);

            return true;
        } break;
        case EVENT_INTERRUPT: {
            int irq_number = arg0;

            if (irq_number < 0 || irq_number >= IC_MAX_IRQS) {
                return false;
            }

            IRQEntry* entry = &state->ic_ram.entries[irq_number];
            if ((entry->flags & IC_FLAGS_ENABLE_MASK) == 0) {
                return false;
            }

            emulator_raise_vector(emulator, entry->cpuid, entry->vector);
            return true;
        } break;
        default:
    }
    return false;
}

bool device_read(HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    IC_State* state = device->state;

    if (size < 1 || size > 4)
        return false;

    if (address >= IC_BASE && address <= IC_BASE + sizeof(IC_RAM) - size) {
        memcpy(data, (char*)&state->ic_ram + address - IC_BASE, size);
        return true;
    }

    return false;
}
bool device_write(HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    EmulatorContext* emulator = device->emulator;
    IC_State* state = device->state;
    if (size < 1 || size > 4)
        return false;
    
    if (address >= IC_BASE && address <= IC_BASE + sizeof(IC_RAM) - size) {
        memcpy((char*)&state->ic_ram + address - IC_BASE, data, size);

        if (address == IC_CONTROL) {
            if (state->ic_ram.control & IC_CONTROL_IPI_SEND_MASK) {
                emulator_raise_vector(emulator, state->ic_ram.ipi_cpuid, state->ic_ram.ipi_vector);
            }
        }

        return true;
    }

    return false;
}
