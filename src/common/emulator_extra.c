
#include "lwm/emulator.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

void emulator_raise_vector(EmulatorContext* emulator, int cpuid, int vector) {
    if (cpuid < 0 || cpuid >= emulator->platformConfig->core_count) {
        printf("WARNING: emulator_raise_vector, cpuid %d does not exist!\n", cpuid);
        return;
    }

    if (vector < MAX_EXCEPTION_VECTORS || vector > MAX_VECTORS) {
        printf("WARNING: emulator_raise_vector, low vectors 0-31 (%d) are not allowed! Function is intended for interrupts.\n", vector);
        return;
    }

    if (emulator->platformConfig->verbose) {
        printf("RAISE cpu=%d vector=%d\n", cpuid, vector);
    }
    CoreState* core = &emulator->cores[cpuid];
    ENABLE_PENDING_VECTOR(core, vector);
}

void emulator_request_interrupt(EmulatorContext* emulator, int irq_number) {
    for (int i=0;i<emulator->platformConfig->devices_len;i++) {
        HardwareDevice* device = emulator->platformConfig->devices[i];
        if (device->queue_interrupt) {
            device->queue_interrupt(emulator, device, irq_number);
            break;
        }
    }
}

void emulator_boot_core(EmulatorContext* emulator, int cpuid, uintptr_t entry) {
    if (cpuid < 0 || cpuid >= emulator->platformConfig->core_count)
        return;

    CoreState* core = &emulator->cores[cpuid];
    if (core->running)
        return;

    core->pc = entry;
    core->crstatus = 0;
    core->crcpuid = cpuid;
    core->crtimercmp = 0;
    core->running = true;
}

void emulator_reset_core(EmulatorContext* emulator, int cpuid) {
    if (cpuid < 0 || cpuid >= emulator->platformConfig->core_count)
        return;

    CoreState* core = &emulator->cores[cpuid];
    core->running = false;
}
