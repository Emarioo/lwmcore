


#include "lwm/emulator.h"
#include "lwm/isa.h"

#include "lwm/util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>



#define PAGING_ENABLED() (core->crstatus & CRSTATUS_PAGING)
#define INTERRUPT_ENABLED() (core->crstatus & CRSTATUS_INTERRUPT)
#define USER_ENABLED() (core->crstatus & CRSTATUS_USER)

// @TODO Misaligned reads
#define  READ8(address) read_address8(core, address)
#define READ16(address) read_address16(core, address)
#define READ32(address) read_address32(core, address)
#define READ64(address) read_address64(core, address)

#define  WRITE8(address, value) write_address8(core, address, value)
#define WRITE16(address, value) write_address16(core, address, value)
#define WRITE32(address, value) write_address32(core, address, value)
#define WRITE64(address, value) write_address64(core, address, value)


#define BITMASK(WORD, BL, BH) ( ((WORD) >> BL) & ( ((uint64_t)1 << ((1+BH)-BL)) - 1 ) )

#define REGISTER_BYTESIZE() (2 << core->mode)

void hard_fault(CoreState* core) {
    core->running = false;
    longjmp(core->loop_jmpbuf, 1);
}

void emulator_trigger_exception(CoreState* core, int vector);
void emulator_enter_vector(CoreState* core, int vector);

void check_registers(CoreState* core, uint8_t* regs, int count) {
    uint64_t pc = core->pc;
    if (core->mode != MODE_16) {
        for (int i=0;i<count;i++) {
            if (regs[i] > 31) {
                printf("Exception ILLEGAL instruction at 0x%zx. Registers limited to 32 (found r%d)\n", pc, regs[i]);
                emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
                return;
            }
        }
    } else {
        for (int i=0;i<count;i++) {
            if (regs[i] > 15) {
                printf("Exception ILLEGAL instruction at 0x%zx. 16-bit mode, upper 16-bit register not available\n", pc);
                emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
                return;
            }
        }
    }
}

void check_privilege(CoreState* core) {
    if (core->crstatus & CRSTATUS_USER) {
        // Tried to execute privileged instruction
        emulator_trigger_exception(core, VECTOR_PROTECTION_FAULT);
    }
}

void access_address(CoreState* core, uint64_t address, uint64_t size) {
    // printf("ACCESS 0x%zx\n", address);
}

bool write_address(CoreState* core, uint64_t address, uint64_t size, void* data, bool physical) {
    EmulatorContext* emulator = core->emulator;
    
    access_address(core, address, size);

    if (!physical) {
        if (PAGING_ENABLED()) {
            printf("Paging not implemented at instruction fetch\n");
            hard_fault(core);
            return false;
        }
    }

    for (int i=0;i<emulator->platformConfig->devices_len;i++) {
        HardwareDevice* dev = emulator->platformConfig->devices[i];
        if (dev->mmio_write) {
            bool res = dev->mmio_write(emulator, dev, address, size, data);
            if (res) {
                return true;
            }
        }
    }

    // We must check both [address:address+size] because of integer overflow.
    if (address > emulator->physicalMemory_size || address + size > emulator->physicalMemory_size) {
        printf("BUS ERROR at 0x%zx, outside physical memory 0x%zx\n", address, emulator->physicalMemory_size);
        emulator_trigger_exception(core, VECTOR_BUS_ERROR);
        return false;
    }

    switch (size) {
        case 1: *(uint8_t*)&emulator->physicalMemory[address] = *(uint8_t*)data; break;
        case 2: *(uint16_t*)&emulator->physicalMemory[address] = *(uint16_t*)data; break;
        case 4: *(uint32_t*)&emulator->physicalMemory[address] = *(uint32_t*)data; break;
        case 8: *(uint64_t*)&emulator->physicalMemory[address] = *(uint64_t*)data; break;
        default: Assert(false);
    }

    return true;
}


bool read_address(CoreState* core, uint64_t address, uint64_t size, void* data, bool physical) {
    EmulatorContext* emulator = core->emulator;
    // @TODO Add write/read access to check

    access_address(core, address, size);

    if (!physical) {
        if (PAGING_ENABLED()) {
            printf("Paging not implemented at instruction fetch\n");
            hard_fault(core);
            return false;
        }
    }

    for (int i=0;i<emulator->platformConfig->devices_len;i++) {
        HardwareDevice* dev = emulator->platformConfig->devices[i];
        if (dev->mmio_read) {
            // printf("DEVR 0x%zx\n", address);
            bool res = dev->mmio_read(emulator, dev, address, size, data);
            if (res) {
                return true;
            }
        }
    }

    if (address > emulator->physicalMemory_size || address + size > emulator->physicalMemory_size) {
        printf("BUS ERROR at 0x%zx, outside physical memory 0x%zx\n", address, emulator->physicalMemory_size);
        emulator_trigger_exception(core, VECTOR_BUS_ERROR);
        return false;
    }
    
    switch (size) {
        case 1: *(uint8_t*)data = *(uint8_t*)&emulator->physicalMemory[address]; break;
        case 2: *(uint16_t*)data = *(uint16_t*)&emulator->physicalMemory[address]; break;
        case 4: *(uint32_t*)data = *(uint32_t*)&emulator->physicalMemory[address]; break;
        case 8: *(uint64_t*)data = *(uint64_t*)&emulator->physicalMemory[address]; break;
        default: Assert(false);
    }

    return true;
}

static inline uint8_t read_address8(CoreState* core, uint64_t address) {
    uint8_t value;
    read_address(core, address, 1, &value, false);
    return value;
}
static inline uint16_t read_address16(CoreState* core, uint64_t address) {
    uint16_t value;
    read_address(core, address, 2, &value, false);
    return value;
}
static inline uint32_t read_address32(CoreState* core, uint64_t address) {
    uint32_t value;
    read_address(core, address, 4, &value, false);
    return value;
}
static inline uint64_t read_address64(CoreState* core, uint64_t address) {
    uint64_t value;
    read_address(core, address, 8, &value, false);
    return value;
}
static inline void write_address8(CoreState* core, uint64_t address, uint8_t value) {
    write_address(core, address, 1, &value, false);
}
static inline void write_address16(CoreState* core, uint64_t address, uint16_t value) {
    write_address(core, address, 2, &value, false);
}
static inline void write_address32(CoreState* core, uint64_t address, uint32_t value) {
    write_address(core, address, 4, &value, false);
}
static inline void write_address64(CoreState* core, uint64_t address, uint64_t value) {
    write_address(core, address, 8, &value, false);
}



int decode_form_reg1_imm(CoreState* core, uint64_t pc, uint32_t opcodeBase, uint32_t* opcode, uint8_t regs[1], uint64_t* immediate) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }
    regs[0] = READ8(pc + 1);

    int immediateSize = 1 << (tmp_opcode - opcodeBase);

    if (immediateSize == 1)
        *immediate = (int64_t)(int8_t)READ8(pc + 2);
    else if (immediateSize == 2)
        *immediate = (int64_t)(int16_t)READ16(pc + 2);
    else if (immediateSize == 4)
        *immediate = (int64_t)(int32_t)READ32(pc + 2);
    else if (immediateSize == 8)
        *immediate = (int64_t)READ64(pc + 2);

    return 2 + immediateSize;
}


int decode_form_reg1(CoreState* core, uint64_t pc, uint32_t* opcode, uint8_t regs[1]) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    regs[0] = READ8(pc+1);

    return 2;
}

int decode_form_reg2(CoreState* core, uint64_t pc, uint32_t* opcode, uint8_t regs[2]) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    regs[0] = READ8(pc+1);
    regs[1] = READ8(pc+2);

    return 3;
}

int decode_form_reg3(CoreState* core, uint64_t pc, uint32_t* opcode, uint8_t regs[3]) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    regs[0] = READ8(pc+1);
    regs[1] = READ8(pc+2);
    regs[2] = READ8(pc+3);

    return 4;
}

int decode_form_reg4(CoreState* core, uint64_t pc, uint32_t* opcode, uint8_t regs[4]) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    regs[0] = READ8(pc+1);
    regs[1] = READ8(pc+2);
    regs[2] = READ8(pc+3);
    regs[3] = READ8(pc+4);

    return 5;
}

int decode_form_pc(CoreState* core, uint64_t pc, uint32_t opcodeBase, uint32_t* opcode, int32_t* immediate) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    int immediateSize = 1 << (tmp_opcode - opcodeBase);

    if (immediateSize == 1)
        *immediate = (int8_t)READ8(pc + 1);
    else if (immediateSize == 2)
        *immediate = (int16_t)READ16(pc + 1);
    else if (immediateSize == 4)
        *immediate = (int32_t)READ32(pc + 1);

    return 1 + immediateSize;
}

int decode_form_jmp_reg1(CoreState* core, uint64_t pc, uint32_t* opcode, uint8_t regs[1], int32_t* relative) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    // [opcode 8 | flags 3 | reg 5 | relative8/16/32 ]
    // flags 3 = relsize 2 | reserved 1

    uint8_t word = (uint8_t)READ8(pc+1);

    int flags = BITMASK(word, 0, 2);
    regs[0]   = BITMASK(word, 3, 7);

    int immediateSize = 1 << (flags & 0x3);

    if (immediateSize == 1)
        *relative = (int8_t)READ8(pc + 2);
    else if (immediateSize == 2)
        *relative = (int16_t)READ16(pc + 2);
    else if (immediateSize == 4)
        *relative = (int32_t)READ32(pc + 2);

    return 2 + immediateSize;
}

int decode_form_jmp_reg2(CoreState* core, uint64_t pc, uint32_t* opcode, ConditionKind* cond, uint8_t regs[2], int32_t* relative) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    // [opcode 8 | flags 6 | reg 5 | reg 5 | relative8/16/32 ]
    // flags 6 = relsize 2 | cond 4

    uint16_t word = (uint16_t)READ8(pc+1) | ((uint16_t)READ8(pc+2) << 8);

    int flags = BITMASK(word, 0, 5);
    regs[0] = BITMASK(word, 6, 10);
    regs[1] = BITMASK(word, 11, 15);

    int immediateSize = 1 << (flags & 0x3);
    int condition     = (flags >> 2) & 0x3F;

    *cond = condition;

    if (immediateSize == 1)
        *relative = (int8_t)READ8(pc + 3);
    else if (immediateSize == 2)
        *relative = (int16_t)READ16(pc + 3);
    else if (immediateSize == 4)
        *relative = (int32_t)READ32(pc + 3);

    return 3 + immediateSize;
}

int decode_form_memory(CoreState* core, uint64_t pc, uint32_t* opcode, uint8_t regs[3], AddressingForm* addressingForm, int64_t* displacement) {
    /*
    lea r0, [r1 + disp8/16/32]
        [ opcode 8 | reg 5 | reg 5 | disp8 ]
        [ opcode 8 | reg 5 | reg 5 | disp16 ]
        [ opcode 8 | reg 5 | reg 5 | disp32 ]

    lea r0, [r1 + r2 + disp8/16/32]
        [ opcode 8 | reg 5 | reg 5 | reg 5 | disp8 ]
        [ opcode 8 | reg 5 | reg 5 | reg 5 | disp16 ]
        [ opcode 8 | reg 5 | reg 5 | reg 5 | disp32 ]

    lea r0, [pc + disp8/16/32]
        [ opcode 8 | reg 5 | disp8/16/64 ]

    lea r0, [16/32/64]
        [ opcode 8 | reg 5 | disp16/32/64 ]

    All fields excluding flags
        [ opcode 8 | reg 5 | reg 5 | reg 5 | disp8/16/32/64 ]

    Base memory encoding
        [ opcode 8 | flags 3 | reg 5 ]
    */

    regs[0] = 0; // Clear since we may not set them and code that checks valid
    regs[1] = 0; // registers won't fail because of uninitialized values.
    regs[2] = 0;

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    uint8_t word = (uint8_t)READ8(pc+1);

    int flags = BITMASK(word, 0, 2);
    regs[0]   = BITMASK(word, 3, 7);

    AddressingForm form = ADDRESSING_ENUM(flags, 0);
    *addressingForm = form;

    if (form == ADDRESSING_ABS16) {
        *displacement = (uint16_t)READ16(pc + 2);
        return largest_encoding(tmp_opcode, form);

    } else if (form == ADDRESSING_ABS32) {
        *displacement = (uint32_t)READ32(pc + 2);
        return largest_encoding(tmp_opcode, form);

    } else if (form == ADDRESSING_ABS64) {
        *displacement = (uint64_t)READ64(pc + 2);
        return largest_encoding(tmp_opcode, form);

    } else if (form == ADDRESSING_REG1_DISP8) {

        uint8_t extraWord = (uint8_t)READ8(pc+2);
        int extraFlags = BITMASK(extraWord, 0, 2);
        regs[1]   = BITMASK(extraWord, 3, 7);

        form = ADDRESSING_ENUM(flags, extraFlags);
        *addressingForm = form;

        if (form == ADDRESSING_REG1_DISP8) {
            *displacement = (int8_t)READ8(pc + 3);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_DISP16) {
            *displacement = (int16_t)READ16(pc + 3);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_DISP32) {
            *displacement = (int32_t)READ32(pc + 3);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_DISP64) {
            *displacement = (int64_t)READ64(pc + 3);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_PC_DISP8) {
            *displacement = (int8_t)READ8(pc + 3);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_PC_DISP16) {
            *displacement = (int16_t)READ16(pc + 3);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_PC_DISP32) {
            *displacement = (int32_t)READ32(pc + 3);
            return largest_encoding(tmp_opcode, form);
        } else {
            emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
        }

    } else if (form == ADDRESSING_REG2_DISP8) {
        
        uint16_t extraWord = (uint16_t)READ16(pc+2);
        int extraFlags = BITMASK(extraWord, 0, 2) | (BITMASK(extraWord, 8, 10) << 3);
        regs[1]   = BITMASK(extraWord, 3, 7);
        regs[2]   = BITMASK(extraWord, 11, 15);
        
        form = ADDRESSING_ENUM(flags, extraFlags);
        *addressingForm = form;

        if (form == ADDRESSING_REG2_DISP8) {
            *displacement = (int8_t)READ8(pc + 4);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG2_DISP16) {
            *displacement = (int16_t)READ16(pc + 4);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG2_DISP32) {
            *displacement = (int32_t)READ32(pc + 4);
            return largest_encoding(tmp_opcode, form);
        } else {
            emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
        }

    } else if (form == ADDRESSING_PC_DISP8) {
        *displacement = (int8_t)READ8(pc + 2);
        return largest_encoding(tmp_opcode, form);
        
    } else if (form == ADDRESSING_PC_DISP16) {
        *displacement = (int16_t)READ16(pc + 2);
        return largest_encoding(tmp_opcode, form);
        
    } else if (form == ADDRESSING_PC_DISP32) {
        *displacement = (int32_t)READ32(pc + 2);
        return largest_encoding(tmp_opcode, form);

    } else {
        emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
    }

    emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
    return 0;
}

void emulator_step(EmulatorContext* emulator, int coreIndex);

void emulator_dump_state(EmulatorContext* emulator) {
    for (int i=0;i<emulator->platformConfig->core_count;i++) {
        CoreState* core = &emulator->cores[i];
        printf("CORE[%d]:\n", i);
        switch (core->mode) {
            case MODE_16: printf(" mode: 16-bit\n"); break;
            case MODE_32: printf(" mode: 32-bit\n"); break;
            case MODE_64: printf(" mode: 64-bit\n"); break;
            default:      printf(" mode: %d\n", core->mode); break;
        }
        printf(" pc: 0x%zx\n", core->pc);
        printf(" tickCounter: %zu\n", core->tickCounter);

        int stride = 4;

        int reg_count = core->mode == MODE_16 ? 16 : 32;

        char regname[32];
        for (int i=0;i<reg_count;i++) {
            gpr_to_string(i, regname);
            printf(" %3s: %zd (0x%zx) ", regname, core->gprs[i], core->gprs[i]);
            if ((i+1) % stride == 0)
                printf("\n");
        }

        for (int i=0;i<reg_count;i++) {
            // @TODO Make function to convert regnum to string
            cr_to_string(i, regname);
            printf(" %10s: %zd (0x%zx) ", regname, core->crs[i], core->crs[i]);
            if ((i+1) % stride == 0)
                printf("\n");
        }
    }
}


void emulator_trigger_exception(CoreState* core, int vector) {
    if (core->insideFault) {
        if (core->insideDoubleFault) {
            printf("DOUBLE FAULT at 0x%zx\n", core->pc);
            hard_fault(core);
        }
        core->insideDoubleFault = true;
    } else {
        core->insideFault = true;
    }
    emulator_enter_vector(core, vector);
    longjmp(core->loop_jmpbuf, 1);
}

void emulator_enter_vector(CoreState* core, int vector) {
    uint64_t ex_handler = 0;
    int entrySize = core->mode == MODE_64 ? 8 : 4;
    uint64_t eh_address = core->crvb + vector * entrySize;

    // printf("EH addr=0x%zx crvb=0x%zx\n", eh_address, core->crvb);

    // @TODO If this fails then it's a triple fault.
    read_address(core, eh_address, entrySize, &ex_handler, true);

    core->crepc = core->pc;
    core->cresp = core->sp;
    core->crestatus = core->crstatus;
    core->pc = ex_handler;
    core->sp = core->crisp;

    core->crstatus &= ~(CRSTATUS_USER|CRSTATUS_INTERRUPT);
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

    core->emulator = emulator;
    core->mode = MODE_16;
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

void emulator_start(PlatformConfig* config) {

    EmulatorContext _ctx = {0};
    EmulatorContext* emulator = &_ctx;

    if (config->core_count == 0) {
        config->core_count = 1;
    }

    emulator->platformConfig = config;
    emulator->physicalMemory_size = 0x100000;
    emulator->physicalMemory = malloc(emulator->physicalMemory_size);
    memset(emulator->physicalMemory, 0xDF, emulator->physicalMemory_size);

    memcpy(emulator->physicalMemory, config->rom, config->rom_len);

    for (int i=0;i<config->devices_len;i++) {
        HardwareDevice* device = config->devices[i];
        device->init(emulator, device);
    }

    emulator->cores = malloc(sizeof(CoreState) * config->core_count);
    memset(emulator->cores, 0, sizeof(CoreState) * config->core_count);

    emulator_boot_core(emulator, 0, emulator->platformConfig->core_entry);

    if (!config->quiet) {
        printf("Start emulator\n");
    }

    int currentCoreIndex = 0;
    int executedInstructions = 0;

    #define INSTRUCTIONS_PER_CORE_TIME_SLICE 5

    while (true) {
        CoreState* core = &emulator->cores[currentCoreIndex];

        // This detects if all cores are inactive.
        // It detects when a core should be yielded so others can run.
        // @TODO Can it be simplified?
        bool quitEmulator = false;
        bool yieldCore = executedInstructions > INSTRUCTIONS_PER_CORE_TIME_SLICE;
        int nextCoreIndex = currentCoreIndex;
        while (!core->running || (yieldCore && executedInstructions != 0)) {
            nextCoreIndex = (nextCoreIndex + 1) % emulator->platformConfig->core_count;
            if (nextCoreIndex == currentCoreIndex && !yieldCore) {
                quitEmulator = true;
                break;
            }
            core = &emulator->cores[nextCoreIndex];
            executedInstructions = 0;
        }
        if (quitEmulator) {
            break;
        }
        currentCoreIndex = nextCoreIndex;

        if (emulator->platformConfig->core_count > 1) {
            executedInstructions++;
        }

        core->tickCounter++;

        for (int i=0;i<config->devices_len;i++) {
            HardwareDevice* device = config->devices[i];
            if (device->tick) {
                device->tick(emulator, device);
            }
        }

        bool canHandleInterrupt = !core->insideFault && !core->insideDoubleFault && (core->crstatus & CRSTATUS_INTERRUPT);

        if (canHandleInterrupt) {
            if (core->tickCounter == core->crtimercmp) {
                printf("TIMER INTERRUPT at 0x%zx tc=%zu\n", core->pc, core->tickCounter);
                emulator_enter_vector(core, VECTOR_TIMER_INTERRUPT);

            } else if (core->interruptLine) {
                printf("INTERRUPT at 0x%zx vector=%d\n", core->pc, core->vectorIndex);
                emulator_enter_vector(core, core->vectorIndex);
            }
        }

        int jmpResult = setjmp(core->loop_jmpbuf);

        if (jmpResult == 0) {
            
            emulator_step(emulator, currentCoreIndex);

        } else {
            // Exception/fault
            continue;
        }
    }
    
    if (!config->quiet) {
        printf("Stop emulator\n");
        emulator_dump_state(emulator);
    }
}

#define INCOMPLETE_INST(...) do { printf(__VA_ARGS__); hard_fault(core); } while (0)

// #define LOG_INST(FMT, ...)
#define LOG_INST(FMT, ...) if (!emulator->platformConfig->quiet && emulator->platformConfig->verbose) { printf("0x%03zx: " FMT, pc __VA_OPT__(,) __VA_ARGS__); }
// #define LOG_INST(FMT, ...) printf("0x%03zx: " FMT, pc __VA_OPT__(,) __VA_ARGS__);

void emulator_step(EmulatorContext* emulator, int cpuid) {
    CoreState* core = &emulator->cores[cpuid];


    uint64_t pc = core->pc;
    uint8_t opcodeByte;
    uint32_t opcode;

    opcodeByte = READ8(pc);

    uint64_t next_pc = -1;
    uint8_t  regs[4];
    uint64_t immediate = 0;
    int32_t  relative = 0;
    int      bytes;

    // printf("Opcode %d\n", opcodeByte);

    opcode = opcodeByte;
    switch (opcode) {
        case OPCODE_LI8:
        case OPCODE_LI16:
        case OPCODE_LI32:
        case OPCODE_LI64:
        {
            bytes = decode_form_reg1_imm(core, pc, OPCODE_LI8, NULL, regs, &immediate);
            check_registers(core, regs, 1);

            // @TODO If we load 8-bit integer we may want to sign extend it.
            //   Does this instruction have flag to sign extend
            //   or is it a separate instruction?

            core->gprs[regs[0]] = immediate;

            next_pc = pc + bytes;

            LOG_INST("li r%d, %zd\n", regs[0], immediate);
        } break;
        case OPCODE_CALL_REG:
        {
            bytes = decode_form_reg1(core, pc, NULL, regs);
            check_registers(core, regs, 1);

            core->lr = pc + bytes;
            next_pc = core->gprs[regs[0]];

            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_JMP_REG:
        {
            bytes = decode_form_reg1(core, pc, NULL, regs);
            check_registers(core, regs, 1);

            next_pc = core->gprs[regs[0]];

            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_NOT:
        {
            bytes = decode_form_reg2(core, pc, NULL, regs);
            check_registers(core, regs, 2);

            uint64_t result;
            uint64_t left = core->gprs[regs[1]];

            switch (opcode) {
                case OPCODE_NOT: result = ~left; break;
            }

            core->gprs[regs[0]] = result;

            next_pc = pc + bytes;

            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_MTCR:
        {
            check_privilege(core);

            bytes = decode_form_reg2(core, pc, NULL, regs);
            // @TODO Check that control register is valid.
            check_registers(core, regs, 2);

            core->crs[regs[0]] = core->gprs[regs[1]];

            next_pc = pc + bytes;

            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_MFCR:
        {
            check_privilege(core);

            bytes = decode_form_reg2(core, pc, NULL, regs);
            // @TODO Check that control register is valid.
            check_registers(core, regs, 2);

            core->gprs[regs[0]] = core->crs[regs[1]];

            next_pc = pc + bytes;
            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_MSCR:
        {
            check_privilege(core);

            bytes = decode_form_reg2(core, pc, NULL, regs);
            // @TODO Check that control register is valid.
            check_registers(core, regs, 2);

            uint64_t tmp = core->gprs[regs[0]];
            core->gprs[regs[0]] = core->crs[regs[1]];
            core->crs[regs[1]] = tmp;

            next_pc = pc + bytes;
            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_UMUL:
        case OPCODE_UDIV:
        case OPCODE_UMOD:
        case OPCODE_SMUL:
        case OPCODE_SDIV:
        case OPCODE_SMOD:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_SHL:
        case OPCODE_SHR:
        {
            bytes = decode_form_reg3(core, pc, NULL, regs);
            check_registers(core, regs, 3);

            uint64_t result;
            uint64_t left;
            uint64_t right;
            int64_t  leftSigned;
            int64_t  rightSigned;

            if (core->mode == MODE_16) {
                left  = (uint16_t)core->gprs[regs[1]];
                right = (uint16_t)core->gprs[regs[2]];
                leftSigned  = (int16_t)(uint16_t)core->gprs[regs[1]];
                rightSigned = (int16_t)(uint16_t)core->gprs[regs[2]];
            } else if (core->mode == MODE_32) {
                left  = (uint32_t)core->gprs[regs[1]];
                right = (uint32_t)core->gprs[regs[2]];
                leftSigned  = (int32_t)(uint32_t)core->gprs[regs[1]];
                rightSigned = (int32_t)(uint32_t)core->gprs[regs[2]];
            } else if (core->mode == MODE_64) {
                left  = (uint64_t)core->gprs[regs[1]];
                right = (uint64_t)core->gprs[regs[2]];
                leftSigned  = (int64_t)(uint64_t)core->gprs[regs[1]];
                rightSigned = (int64_t)(uint64_t)core->gprs[regs[2]];
            }
            switch (opcode) {
                case OPCODE_ADD:  result = left + right; break;
                case OPCODE_SUB:  result = left - right; break;
                case OPCODE_UMUL: result = left * right; break;
                case OPCODE_UDIV: result = left / right; break;
                case OPCODE_UMOD: result = left % right; break;
                case OPCODE_SMUL: result = leftSigned * rightSigned; break;
                case OPCODE_SDIV: result = leftSigned / rightSigned; break;
                case OPCODE_SMOD: result = leftSigned % rightSigned; break;
                case OPCODE_AND:  result = left & right; break;
                case OPCODE_OR:   result = left | right; break;
                case OPCODE_XOR:  result = left ^ right; break;
                case OPCODE_SHL:  result = left << right; break;
                case OPCODE_SHR:  result = left >> right; break;
            }

            core->gprs[regs[0]] = result;

            next_pc = pc + bytes;

            LOG_INST("%s r%d = %zd\n", opcode_to_string(opcode), regs[0], result);
        } break;
        case OPCODE_JMP:
        case OPCODE_JMP1:
        case OPCODE_JMP2:
        {
            bytes = decode_form_pc(core, pc, OPCODE_JMP, NULL, &relative);

            next_pc = pc + bytes + relative;
            LOG_INST("jmp\n");
        } break;
        case OPCODE_CALL:
        case OPCODE_CALL1:
        case OPCODE_CALL2:
        {
            bytes = decode_form_pc(core, pc, OPCODE_CALL, NULL, &relative);

            core->lr = pc + bytes;
            next_pc = pc + bytes + relative;

            LOG_INST("call lr=0x%zx\n", core->lr);
        } break;
        case OPCODE_RET:
        {
            next_pc = core->lr;
            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_NOP:
        {
            next_pc = pc + 1;
            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_PUSH:
        case OPCODE_POP:
        {
            bytes = decode_form_reg1(core, pc, NULL, regs);
            check_registers(core, regs, 1);

            // printf("push/pop[%d]  %d 0x%x\n", opcode, regs[0], (int)pc);
            
            if (opcode == OPCODE_PUSH) {
                core->sp -= REGISTER_BYTESIZE();
                switch (core->mode) {
                    case MODE_16: WRITE16(core->sp, core->gprs[regs[0]]); break;
                    case MODE_32: WRITE32(core->sp, core->gprs[regs[0]]); break;
                    case MODE_64: WRITE64(core->sp, core->gprs[regs[0]]); break;
                }
            }
            if (opcode == OPCODE_POP) {
                switch (core->mode) {
                    case MODE_16: core->gprs[regs[0]] = READ16(core->sp); break;
                    case MODE_32: core->gprs[regs[0]] = READ32(core->sp); break;
                    case MODE_64: core->gprs[regs[0]] = READ64(core->sp); break;
                }
                core->sp += REGISTER_BYTESIZE();
            }

            next_pc = pc + bytes;
            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_RDTICK:
        case OPCODE_RDTICK1:
        case OPCODE_RDTICK2:
        {
            uint64_t tickCounter = core->tickCounter;

            if (opcode == OPCODE_RDTICK) {
                bytes = decode_form_reg4(core, pc, NULL, regs);
                check_registers(core, regs, 4);

                core->gprs[regs[0]] = (tickCounter >>  0) & 0xFFFF;
                core->gprs[regs[1]] = (tickCounter >> 16) & 0xFFFF;
                core->gprs[regs[2]] = (tickCounter >> 32) & 0xFFFF;
                core->gprs[regs[3]] = (tickCounter >> 48) & 0xFFFF;
            } else if (opcode == OPCODE_RDTICK1) {
                bytes = decode_form_reg2(core, pc, NULL, regs);
                check_registers(core, regs, 2);

                core->gprs[regs[0]] = (tickCounter >>  0) & 0xFFFFFFFF;
                core->gprs[regs[1]] = (tickCounter >> 32) & 0xFFFFFFFF;
            } else if (opcode == OPCODE_RDTICK2) {
                bytes = decode_form_reg1(core, pc, NULL, regs);
                check_registers(core, regs, 1);
                
                core->gprs[regs[0]] = tickCounter;
            }

            next_pc = pc + bytes;

            LOG_INST("rdtick %zu, tc=%zu\n", core->gprs[regs[0]], tickCounter);
        } break;
        case OPCODE_JZ:
        case OPCODE_JNZ:
        {
            bytes = decode_form_jmp_reg1(core, pc, NULL, regs, &relative);
            check_registers(core, regs, 1);

            uint64_t result;
            if (core->mode == MODE_16) {
                result = (uint16_t)core->gprs[regs[0]];
            } else if (core->mode == MODE_32) {
                result = (uint32_t)core->gprs[regs[0]];
            } else if (core->mode == MODE_64) {
                result = (uint64_t)core->gprs[regs[0]];
            }

            if ((result == 0) == (opcode == OPCODE_JZ) ) {
                next_pc = pc + bytes + relative;
            } else {
                next_pc = pc + bytes;
            }
            LOG_INST("%s r%d = %zd\n", opcode_to_string(opcode), regs[0], result);
        } break;
        case OPCODE_JCOND:
        {
            ConditionKind cond;
            bytes = decode_form_jmp_reg2(core, pc, NULL, &cond, regs, &relative);
            check_registers(core, regs, 2);

            bool result;
            uint64_t left;
            uint64_t right;
            int64_t  leftSigned;
            int64_t  rightSigned;

            if (core->mode == MODE_16) {
                left  = (uint16_t)core->gprs[regs[0]];
                right = (uint16_t)core->gprs[regs[1]];
                leftSigned  = (int16_t)(uint16_t)core->gprs[regs[0]];
                rightSigned = (int16_t)(uint16_t)core->gprs[regs[1]];
            } else if (core->mode == MODE_32) {
                left  = (uint32_t)core->gprs[regs[0]];
                right = (uint32_t)core->gprs[regs[1]];
                leftSigned  = (int32_t)(uint32_t)core->gprs[regs[0]];
                rightSigned = (int32_t)(uint32_t)core->gprs[regs[1]];
            } else if (core->mode == MODE_64) {
                left  = (uint64_t)core->gprs[regs[0]];
                right = (uint64_t)core->gprs[regs[1]];
                leftSigned  = (int64_t)(uint64_t)core->gprs[regs[0]];
                rightSigned = (int64_t)(uint64_t)core->gprs[regs[1]];
            }

            switch (cond) {
                case COND_EQ: result = left == right; break;
                case COND_NE: result = left != right; break;
                case COND_LT: result = leftSigned < rightSigned; break;
                case COND_LE: result = leftSigned <= rightSigned; break;
                case COND_GT: result = leftSigned > rightSigned; break;
                case COND_GE: result = leftSigned >= rightSigned; break;
                case COND_A:  result = left < right; break;
                case COND_AE: result = left <= right; break;
                case COND_B:  result = left > right; break;
                case COND_BE: result = left >= right; break;
            }

            if (result) {
                next_pc = pc + bytes + relative;
            } else {
                next_pc = pc + bytes;
            }
            LOG_INST("%s %d\n", opcode_to_string(opcode), result);
        } break;
        case OPCODE_LEA:
        case OPCODE_LDB:
        case OPCODE_LDBS:
        case OPCODE_LDH:
        case OPCODE_LDHS:
        case OPCODE_LDL:
        case OPCODE_LDLS:
        case OPCODE_LDQ:
        case OPCODE_STB:
        case OPCODE_STH:
        case OPCODE_STL:
        case OPCODE_STQ:
        {
            AddressingForm addressingForm;
            int64_t displacement;
            bytes = decode_form_memory(core, pc, NULL, regs, &addressingForm, &displacement);
            check_registers(core, regs, 3);

            uint64_t result;
            uint64_t address;

            switch (addressingForm) {
                // @TODO Consider removing absolute form. Only used for booting and OS stuff.
                //   Right now the encoding isn't optimized so we don't lose anything by
                //   providing it.
                case ADDRESSING_ABS16:
                case ADDRESSING_ABS32:
                case ADDRESSING_ABS64:
                    address = (uint64_t)displacement;
                break;
                case ADDRESSING_REG1_DISP8:
                case ADDRESSING_REG1_DISP16:
                case ADDRESSING_REG1_DISP32:
                case ADDRESSING_REG1_DISP64:
                    address = core->gprs[regs[1]] + displacement;
                break;
                case ADDRESSING_REG2_DISP8:
                case ADDRESSING_REG2_DISP16:
                case ADDRESSING_REG2_DISP32:
                case ADDRESSING_REG2_DISP64:
                    address = core->gprs[regs[1]] + core->gprs[regs[2]] + displacement;
                break;
                case ADDRESSING_PC_DISP8:
                case ADDRESSING_PC_DISP16:
                case ADDRESSING_PC_DISP32:
                    address = pc + bytes + displacement;
                break;
                case ADDRESSING_REG1_PC_DISP8:
                case ADDRESSING_REG1_PC_DISP16:
                case ADDRESSING_REG1_PC_DISP32:
                    address = core->gprs[regs[1]] + pc + bytes + displacement;
                break;
            }

            uint64_t value = core->gprs[regs[0]]; // Here for debugging. Only needed for stores.
            
            switch (opcode) {
                case OPCODE_LEA:  result = address; break;
                case OPCODE_LDB:  result =          (uint8_t)READ8( address); break;
                case OPCODE_LDBS: result =  (int64_t)(int8_t)READ8( address); break;
                case OPCODE_LDH:  result =         (uint16_t)READ16(address); break;
                case OPCODE_LDHS: result = (int64_t)(int16_t)READ16(address); break;
                case OPCODE_LDL:  result =         (uint32_t)READ32(address); break;
                case OPCODE_LDLS: result = (int64_t)(int32_t)READ32(address); break;
                case OPCODE_LDQ:  result =         (uint64_t)READ64(address); break;
                case OPCODE_STB:  WRITE8( address,  (uint8_t)value); break;
                case OPCODE_STH:  WRITE16(address, (uint16_t)value); break;
                case OPCODE_STL:  WRITE32(address, (uint32_t)value); break;
                case OPCODE_STQ:  WRITE64(address, (uint64_t)value); break;
            }

            next_pc = pc + bytes;

            if (opcode >= OPCODE_LEA && opcode <= OPCODE_LDQ) {
                core->gprs[regs[0]] = result;
                LOG_INST("%s r%d = %zd, addr=0x%zx\n", opcode_to_string(opcode), regs[0], result, address);
            } else {
                LOG_INST("%s r%d = %zd, addr=0x%zx\n", opcode_to_string(opcode), regs[0], value, address);
            }
        } break;
        case OPCODE_SYSCALL:
        {
            pc = pc + 1; // We must set PC in here since trigger exception
                         // moves it to CREPC
            LOG_INST("%s\n", opcode_to_string(opcode));
            emulator_trigger_exception(core, VECTOR_SYSCALL);
        } break;
        case OPCODE_VRET:
        {
            check_privilege(core);

            next_pc = core->crepc;
            core->sp = core->cresp;
            core->crstatus = core->crestatus;
            
            core->insideFault = false;
            core->insideDoubleFault = false;
            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_DBG:
        {
            pc = pc + 1; // We must set PC in here since trigger exception
                         // moves it to CREPC
            LOG_INST("%s\n", opcode_to_string(opcode));
            emulator_trigger_exception(core, VECTOR_BREAKPOINT);
        } break;
        case OPCODE_EINT:
        {
            check_privilege(core);

            core->crstatus |= CRSTATUS_INTERRUPT;

            next_pc = pc + 1;
            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_DINT:
        {
            check_privilege(core);

            core->crstatus &= ~CRSTATUS_INTERRUPT;

            next_pc = pc + 1;
            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_SLOW:
        {
            // @TODO What would a proper implementation of slow be?
            //   Since sleeping here will block all cores from running
            //   We should yield the core instead for some time.

            LOG_INST("%s\n", opcode_to_string(opcode));
            sleep_us(100000);
            
            next_pc = pc + 1;
        } break;
        case OPCODE_WFI:
        {
            check_privilege(core);

            LOG_INST("%s\n", opcode_to_string(opcode));
            INCOMPLETE_INST("TODO implement wfi\n");
        } break;
        case OPCODE_TLBFLUSH:
        {
            check_privilege(core);

            LOG_INST("%s\n", opcode_to_string(opcode));
            INCOMPLETE_INST("TODO implement tlbflush\n");
        } break;
        case OPCODE_CPUFEAT:
        {
            LOG_INST("%s\n", opcode_to_string(opcode));
            INCOMPLETE_INST("TODO implement cpufeat\n");
        } break;
        case OPCODE_VMSTART:
        {
            check_privilege(core);

            LOG_INST("%s\n", opcode_to_string(opcode));
            INCOMPLETE_INST("TODO implement vmstart\n");
        } break;
        default: {
            printf("Exception ILLEGAL instruction at 0x%zx. Unknown opcode %d\n", pc, opcode);
            emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
        }
    }

    if (core->running) {
        if (core->pc == next_pc) {
            // @TODO Quit if this happens to often?
            //   Maybe you did a spinloop? Maybe quit unless you called slow or wfi?
            // printf("WARNING: Executing same instruction %zx?\n", next_pc);
        }
        core->pc = next_pc;
    }

    
    // sleep_us(100 * 1000);
}



char tohex(int x) {
    if(x >= 0 && x <= 9)
        return '0' + x;
    if(x >= 10 && x <= 15)
        return 'A' + x - 10;
    return '0';
}

// @TODO Disassembler. Also used in emulator, printing each instruction that runs and updated registers.

// @TODO Disassemble at a specific position

void dump(PlatformConfig* config) {
    uint8_t* rom = config->rom;

    printf("Core entry: %zu\n", config->core_entry);

    int stride = 4;
    
    printf("ROM %d bytes (0x%x)\n", config->rom_len, config->rom_len);
    int i = 0;
    for(;i<config->rom_len;i++) {

        if (i % stride == 0) {
            printf(" 0x%x: ", i);
        }

        char c0 = tohex((rom[i]>>0)&0xF);
        char c1 = tohex((rom[i]>>4)&0xF);
        printf("%c%c ", c1, c0);
        
        if ((i+1) % stride == 0) {
            printf("\n");
        }
    }
    if ((i) % stride != 0) {
        printf("\n");
    }
}

