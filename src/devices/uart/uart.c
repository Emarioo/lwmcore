

#include "lwm/emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>


//########################
//    Memory Mapped IO
//########################


#define UART_BASE       0xE120
#define UART_STATUS     UART_BASE+0x0
#define UART_CONTROL    UART_BASE+0x4
#define UART_DATA       UART_BASE+0x8
#define UART_IRQ        UART_BASE+0xC // Read-only

// Status register
#define UART_TX_READY_MASK  0x2
#define UART_TX_READY_BIT   1
#define UART_RX_READY_MASK  0x4
#define UART_RX_READY_BIT   2

// Control register
#define UART_TX_INTERRUPT_ENABLE_MASK   0x2
#define UART_TX_INTERRUPT_DISABLE_MASK  0xFD
#define UART_RX_INTERRUPT_ENABLE_MASK   0x4
#define UART_RX_INTERRUPT_DISABLE_MASK  0xFB

#define UART_IRQ_NUMBER 1


//########################
//    Device Functions
//########################
bool device_write(HardwareDevice* device, uintptr_t address, size_t size, void* data);
bool device_read(HardwareDevice* device, uintptr_t address, size_t size, void* data);
bool device_tick(HardwareDevice* device);

//########################
//    Implementation
//########################


typedef struct {
    uint32_t status;
    uint32_t control;
    uint32_t data;
    uint32_t irq;
} UART_RAM;

typedef struct {
    UART_RAM uart_ram;
    bool sentIRQ;
} UART_State;


bool device_event(HardwareDevice* device, HardwareEvent eventType, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
    EmulatorContext* emulator = device->emulator;
    UART_State*      state    = device->state;

    switch (eventType) {
        case EVENT_INIT: {
            state = malloc(sizeof(*state));
            memset(state, 0, sizeof(*state));
            device->state     = state;
            device->eventMask = 0;
            device->mmio_read = device_read;
            device->mmio_write = device_write;
            device->tick = device_tick;

            declare_mmio(device, UART_BASE, 0x20);

            state->uart_ram.status = UART_TX_READY_MASK;

            // Turn off line buffering, must be restored when program exits
            // struct termios t;
            // tcgetattr(0, &t);
            // t.c_lflag &= ~(ICANON | ECHO);
            // t.c_cc[VMIN] = 0;
            // t.c_cc[VTIME] = 0;
            // tcsetattr(0, TCSANOW, &t);

            return true;
        } break;
        default:
    }
    return false;
}


bool device_read(HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    UART_State*      state    = device->state;

    if (size < 1 || size > 4)
        return false;

    if (size == 2 && address == UART_IRQ) {
        *(uint16_t*)data = UART_IRQ_NUMBER;
        return true;
    }
    
    if (size == 1 && address == UART_DATA) {
        state->sentIRQ = false;
        
        *(char*)data = 0;
        struct pollfd pfd;
        pfd.fd = 0; // stdin
        pfd.events = POLLIN;
        int res = poll(&pfd, 1, 0); // 0 = non-blocking
        if (res > 0) {
            res = read(0, data, 1);
            if (res != 1) {
                printf("UART read failed %d\n", res);
            } else {
                int res = poll(&pfd, 1, 0); // 0 = non-blocking
                if (res > 0) {
                    state->uart_ram.status |= UART_RX_READY_MASK;
                } else {
                    state->uart_ram.status &= ~UART_RX_READY_MASK;
                }
            }
        }
        return true;
    }

    if (address >= UART_BASE && address <= UART_BASE + sizeof(UART_RAM) - size) {
        memcpy(data, (char*)&state->uart_ram + address - UART_BASE, size);
        return true;
    }

    return false;
}

bool device_write(HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    UART_State*      state    = device->state;
    if (size < 1 || size > 4)
        return false;

    if (address == UART_CONTROL) {
        memcpy(&state->uart_ram.control, data, size);
        // @TODO Check if we enabled interrupts
        return true;
    }
    
    if (size == 1 && address == UART_DATA) {
        fwrite(data, 1, size, stdout);
        fflush(stdout);
        state->uart_ram.status = UART_TX_READY_MASK;
        return true;
    }
    
    return false;
}

bool device_tick(HardwareDevice* device) {
    UART_State*      state    = device->state;
    EmulatorContext* emulator = device->emulator;
    
    struct pollfd pfd;
    pfd.fd = 0; // stdin
    pfd.events = POLLIN;
    int res = poll(&pfd, 1, 0); // 0 = non-blocking
    if (res > 0) {
        // @TODO Put char in a buffer so we stop sending IRQs.
        //   Potentially set a flag to disable requests until 
        //   RX is read from
        state->uart_ram.status |= UART_RX_READY_MASK;
        if (state->uart_ram.control & UART_RX_INTERRUPT_ENABLE_MASK) {
            if (!state->sentIRQ) {
                state->sentIRQ = true;
                emulator_request_interrupt(emulator, UART_IRQ_NUMBER);
            }
        }
    }

    return true;
}
