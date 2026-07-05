


#include "lwm/emulator.h"
#include "lwm/isa.h"

#include "lwm/util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <setjmp.h>

typedef struct {
    CoreState coreState;
    PlatformConfig* platformConfig;
    
    uint8_t* physicalMemory;
    uint64_t physicalMemory_size;

    bool running;

    jmp_buf loop_jmpbuf;

} EmulatorContext;


#define PAGING_ENABLED() (core->crstatus & CRSTATUS_PAGING)
#define INTERRUPT_ENABLED() (core->crstatus & CRSTATUS_INTERRUPT)
#define USER_ENABLED() (core->crstatus & CRSTATUS_USER)

// @TODO Misaligned reads
#define  READ8(address) read_address8(emulator, address)
#define READ16(address) read_address16(emulator, address)
#define READ32(address) read_address32(emulator, address)
#define READ64(address) read_address64(emulator, address)

#define  WRITE8(address, value) write_address8(emulator, address, value)
#define WRITE16(address, value) write_address16(emulator, address, value)
#define WRITE32(address, value) write_address32(emulator, address, value)
#define WRITE64(address, value) write_address64(emulator, address, value)


#define BITMASK(WORD, BL, BH) ( ((WORD) >> BL) & ( ((uint64_t)1 << ((1+BH)-BL)) - 1 ) )

#define REGISTER_BYTESIZE() (2 << emulator->coreState.mode)

void fault(EmulatorContext* emulator) {
    longjmp(emulator->loop_jmpbuf, 1);
}


void check_registers(EmulatorContext* emulator, uint8_t* regs, int count) {
    uint64_t pc = emulator->coreState.pc;
    if (emulator->coreState.mode != MODE_16) {
        for (int i=0;i<count;i++) {
            if (regs[i] > 31) {
                fprintf(stderr, "Exception ILLEGAL instruction at 0x%zx. Registers limited to 32 (found r%d)\n", pc, regs[i]);
                fault(emulator);
                return;
            }
        }
    } else {
        for (int i=0;i<count;i++) {
            if (regs[i] > 15) {
                fprintf(stderr, "Exception ILLEGAL instruction at 0x%zx. 16-bit mode, upper 16-bit register not available\n", pc);
                fault(emulator);
                return;
            }
        }
    }
}

bool write_address(EmulatorContext* emulator, uint64_t address, uint64_t size, void* data) {
    CoreState* core = &emulator->coreState;
    
    if (PAGING_ENABLED()) {
        fprintf(stderr, "Paging not implemented at instruction fetch\n");
        fault(emulator);
        return false;
    }

    for (int i=0;i<emulator->platformConfig->devices_len;i++) {
        HardwareDevice* dev = emulator->platformConfig->devices[i];
        if (dev->mmio_write) {
            bool res = dev->mmio_write(address, size, data);
            if (res) {
                return true;
            }
        }
    }

    if (address + size > emulator->physicalMemory_size) {
        // halt
        fprintf(stderr, "Halt at 0x%zx, outside physical memory 0x%zx\n", address, emulator->physicalMemory_size);
        fault(emulator);
        return false;
    }

    switch (size) {
        case 1: *(uint8_t*)&emulator->physicalMemory[address] = *(uint8_t*)data; break;
        case 2: *(uint16_t*)&emulator->physicalMemory[address] = *(uint16_t*)data; break;
        case 4: *(uint32_t*)&emulator->physicalMemory[address] = *(uint32_t*)data; break;
        case 8: *(uint64_t*)&emulator->physicalMemory[address] = *(uint64_t*)data; break;
        default: Assert(false);
    }

    // printf("ACCESS 0x%zx\n", address);

    return true;
}


bool read_address(EmulatorContext* emulator, uint64_t address, uint64_t size, void* data) {
    // @TODO Add write/read access to check

    CoreState* core = &emulator->coreState;
    if (PAGING_ENABLED()) {
        fprintf(stderr, "Paging not implemented at instruction fetch\n");
        fault(emulator);
        return false;
    }

    for (int i=0;i<emulator->platformConfig->devices_len;i++) {
        HardwareDevice* dev = emulator->platformConfig->devices[i];
        if (dev->mmio_read) {
            bool res = dev->mmio_read(address, size, data);
            if (res) {
                return true;
            }
        }
    }

    if (address + size > emulator->physicalMemory_size) {
        // halt
        fprintf(stderr, "Halt at 0x%zx, outside physical memory 0x%zx\n", address, emulator->physicalMemory_size);
        fault(emulator);
        return false;
    }
    
    switch (size) {
        case 1: *(uint8_t*)data = *(uint8_t*)&emulator->physicalMemory[address]; break;
        case 2: *(uint16_t*)data = *(uint16_t*)&emulator->physicalMemory[address]; break;
        case 4: *(uint32_t*)data = *(uint32_t*)&emulator->physicalMemory[address]; break;
        case 8: *(uint64_t*)data = *(uint64_t*)&emulator->physicalMemory[address]; break;
        default: Assert(false);
    }

    // printf("ACCESS 0x%zx\n", address);

    return true;
}

static inline uint8_t read_address8(EmulatorContext* emulator, uint64_t address) {
    uint8_t value;
    read_address(emulator, address, 1, &value);
    return value;
}
static inline uint16_t read_address16(EmulatorContext* emulator, uint64_t address) {
    uint16_t value;
    read_address(emulator, address, 2, &value);
    return value;
}
static inline uint32_t read_address32(EmulatorContext* emulator, uint64_t address) {
    uint32_t value;
    read_address(emulator, address, 4, &value);
    return value;
}
static inline uint64_t read_address64(EmulatorContext* emulator, uint64_t address) {
    uint64_t value;
    read_address(emulator, address, 8, &value);
    return value;
}
static inline void write_address8(EmulatorContext* emulator, uint64_t address, uint8_t value) {
    write_address(emulator, address, 1, &value);
}
static inline void write_address16(EmulatorContext* emulator, uint64_t address, uint16_t value) {
    write_address(emulator, address, 2, &value);
}
static inline void write_address32(EmulatorContext* emulator, uint64_t address, uint32_t value) {
    write_address(emulator, address, 4, &value);
}
static inline void write_address64(EmulatorContext* emulator, uint64_t address, uint64_t value) {
    write_address(emulator, address, 8, &value);
}



int decode_form_reg1_imm(EmulatorContext* emulator, uint64_t pc, uint32_t opcodeBase, uint32_t* opcode, uint8_t regs[1], uint64_t* immediate) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }
    regs[0] = READ8(pc + 1);

    int immediateSize = 1 << (tmp_opcode - opcodeBase);

    if (immediateSize == 1)
        *immediate = READ8(pc + 2);
    else if (immediateSize == 2)
        *immediate = READ16(pc + 2);
    else if (immediateSize == 4)
        *immediate = READ32(pc + 2);
    else if (immediateSize == 8)
        *immediate = READ64(pc + 2);

    return 2 + immediateSize;
}


int decode_form_reg1(EmulatorContext* emulator, uint64_t pc, uint32_t* opcode, uint8_t regs[1]) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    regs[0] = READ8(pc+1);

    return 2;
}

int decode_form_reg2(EmulatorContext* emulator, uint64_t pc, uint32_t* opcode, uint8_t regs[2]) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    regs[0] = READ8(pc+1);
    regs[1] = READ8(pc+2);

    return 3;
}

int decode_form_reg3(EmulatorContext* emulator, uint64_t pc, uint32_t* opcode, uint8_t regs[3]) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    regs[0] = READ8(pc+1);
    regs[1] = READ8(pc+2);
    regs[2] = READ8(pc+3);

    return 4;
}

int decode_form_reg4(EmulatorContext* emulator, uint64_t pc, uint32_t* opcode, uint8_t regs[4]) {

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

int decode_form_pc(EmulatorContext* emulator, uint64_t pc, uint32_t opcodeBase, uint32_t* opcode, int32_t* immediate) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    int immediateSize = 1 << (tmp_opcode - opcodeBase);

    if (immediateSize == 1)
        *immediate = READ8(pc + 1);
    else if (immediateSize == 2)
        *immediate = READ16(pc + 1);
    else if (immediateSize == 4)
        *immediate = READ32(pc + 1);

    return 1 + immediateSize;
}

int decode_form_jmp_reg1(EmulatorContext* emulator, uint64_t pc, uint32_t* opcode, uint8_t regs[1], int32_t* relative) {

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

    return 3 + immediateSize;
}

int decode_form_jmp_reg2(EmulatorContext* emulator, uint64_t pc, uint32_t* opcode, ConditionKind* cond, uint8_t regs[2], int32_t* relative) {

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

int decode_form_memory(EmulatorContext* emulator, uint64_t pc, uint32_t* opcode, uint8_t regs[3], AddressingForm* addressingForm, int64_t* displacement) {
    /*
    lea r0, [r1 + disp8/16/32]
        [ opcode 8 | reg 5 | reg 5 | disp8 ]
        [ opcode 8 | reg 5 | reg 5 | disp16 ]
        [ opcode 8 | reg 5 | reg 5 | disp32 ]

    lea r0, [r1 + r2 + disp8/16/32]
        [ opcode 8 | reg 5 | reg 5 | reg 5 | disp8 ]
        [ opcode 8 | reg 5 | reg 5 | reg 5 | disp16 ]
        [ opcode 8 | reg 5 | reg 5 | reg 5 | disp32 ]

    lea r0, [pc + disp32]
        [ opcode 8 | reg 5 | disp32 ]

    lea r0, [16/32/64]
        [ opcode 8 | reg 5 | disp16 ]
        [ opcode 8 | reg 5 | disp32 ]
        [ opcode 8 | reg 5 | disp64 ]

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

    if (flags == 0) {

        *addressingForm = ADDRESSING_ABS16;
        *displacement = (int16_t)READ16(pc + 2);
        return 4;

    } else if (flags == 1) {

        *addressingForm = ADDRESSING_ABS32;
        *displacement = (int32_t)READ32(pc + 2);
        return 6;

    } else if (flags == 2) {

        *addressingForm = ADDRESSING_ABS64;
        *displacement = (int64_t)READ64(pc + 2);
        return 10;

    } else if (flags == 3) {

        uint8_t extraWord = (uint8_t)READ8(pc+2);
        int extraFlags = BITMASK(extraWord, 0, 2);
        regs[1]   = BITMASK(extraWord, 3, 7);

        if (extraFlags == 0) {
            *addressingForm = ADDRESSING_REG1_DISP8;
            *displacement = (int8_t)READ8(pc + 3);
            return 4;
        } else if (extraFlags == 1) {
            *addressingForm = ADDRESSING_REG1_DISP16;
            *displacement = (int16_t)READ16(pc + 3);
            return 5;
        } else if (extraFlags == 2) {
            *addressingForm = ADDRESSING_REG1_DISP32;
            *displacement = (int32_t)READ32(pc + 3);
            return 7;
        } else {
            // @TODO illegal instruction
            assert(false);
        }

    } else if (flags == 4) {
        
        uint16_t extraWord = (uint16_t)READ16(pc+2);
        int extraFlags = BITMASK(extraWord, 0, 2) | (BITMASK(extraWord, 8, 10) << 3);
        regs[1]   = BITMASK(extraWord, 3, 7);
        regs[2]   = BITMASK(extraWord, 11, 15);

        if (extraFlags == 0) {
            *addressingForm = ADDRESSING_REG2_DISP8;
            *displacement = (int8_t)READ8(pc + 4);
            return 5;
        } else if (extraFlags == 1) {
            *addressingForm = ADDRESSING_REG2_DISP16;
            *displacement = (int16_t)READ16(pc + 4);
            return 6;
        } else if (extraFlags == 2) {
            *addressingForm = ADDRESSING_REG2_DISP32;
            *displacement = (int32_t)READ32(pc + 4);
            return 8;
        } else {
            // @TODO illegal instruction
            assert(false);
        }

    } else if (flags == 5) {
        
        *addressingForm = ADDRESSING_PC_DISP32;
        *displacement = (int32_t)READ32(pc + 2);
        return 6;

    } else {
        // @TODO illegal instruction
        assert(false);
    }

    fault(emulator);
    return 0;
}

void emulator_step(EmulatorContext* emulator);

void emulator_dump_state(EmulatorContext* emulator) {
    CoreState* core = &emulator->coreState;

    printf("Core state:\n");
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


bool uart_mmio_write(uintptr_t address, size_t size, void* data) {
    if (size == 1 && address == 0x2000) {
        printf("%c", *(char*)data);
        return true;
    }
    return false;
}

static HardwareDevice uart_device = {
    .mmio_write = uart_mmio_write   
};

void emulator_start(PlatformConfig* config) {

    EmulatorContext _ctx = {0};
    EmulatorContext* emulator = &_ctx;


    emulator->platformConfig = config;
    emulator->physicalMemory_size = 0x100000;
    emulator->physicalMemory = malloc(emulator->physicalMemory_size);
    memset(emulator->physicalMemory, 0xDF, emulator->physicalMemory_size);

    memcpy(emulator->physicalMemory, config->rom, config->rom_len);

    emulator->coreState.mode = MODE_16;
    emulator->coreState.pc = emulator->platformConfig->core_entry;

    if (emulator->platformConfig->devices_len == 0) {
        emulator->platformConfig->devices = malloc(sizeof(HardwareDevice));
        emulator->platformConfig->devices[0] = &uart_device;
        emulator->platformConfig->devices_len = 1;
    }

    CoreState* core = &emulator->coreState;

    emulator->running = true;

    printf("Start emulator\n");

    while (emulator->running) {

        int jmpResult = setjmp(emulator->loop_jmpbuf);

        if (jmpResult == 0) {
            emulator_step(emulator);
        } else {
            // exception
            emulator->running = false;
            break;
        }

    }
    
    printf("Stop emulator\n");

    emulator_dump_state(emulator);
}

#define INCOMPLETE_INST(...) do { fprintf(stderr, __VA_ARGS__); fault(emulator); } while (0)

void emulator_step(EmulatorContext* emulator) {
    CoreState* core = &emulator->coreState;

    uint64_t pc = core->pc;
    uint8_t opcodeByte;
    uint32_t opcode;

    opcodeByte = READ8(pc);

    uint64_t next_pc = -1;
    uint8_t  regs[4];
    uint64_t immediate;
    int32_t  relative;
    int      bytes;

    // printf("Opcode %d\n", opcodeByte);

    opcode = opcodeByte;
    switch (opcode) {
        case OPCODE_LI8:
        case OPCODE_LI16:
        case OPCODE_LI32:
        case OPCODE_LI64:
        {
            bytes = decode_form_reg1_imm(emulator, pc, OPCODE_LI8, NULL, regs, &immediate);
            check_registers(emulator, regs, 1);

            // @TODO If we load 8-bit integer we may want to sign extend it.
            //   Does this instruction have flag to sign extend
            //   or is it a separate instruction?

            core->gprs[regs[0]] = immediate;

            next_pc = pc + bytes;
        } break;
        case OPCODE_CALL_REG:
        {
            bytes = decode_form_reg1(emulator, pc, NULL, regs);
            check_registers(emulator, regs, 1);

            core->lr = pc + bytes;
            next_pc = core->gprs[regs[0]];
        } break;
        case OPCODE_JMP_REG:
        {
            bytes = decode_form_reg1(emulator, pc, NULL, regs);
            check_registers(emulator, regs, 1);

            next_pc = core->gprs[regs[0]];
        } break;
        case OPCODE_NOT:
        {
            bytes = decode_form_reg2(emulator, pc, NULL, regs);
            check_registers(emulator, regs, 2);

            uint64_t result;
            uint64_t left = core->gprs[regs[1]];

            switch (opcode) {
                case OPCODE_NOT: result = ~left; break;
            }

            core->gprs[regs[0]] = result;

            next_pc = pc + bytes;
        } break;
        case OPCODE_MTCR:
        {
            bytes = decode_form_reg2(emulator, pc, NULL, regs);
            // @TODO Check that control register is valid.
            check_registers(emulator, regs, 2);

            core->crs[regs[0]] = core->gprs[regs[1]];

            next_pc = pc + bytes;
        } break;
        case OPCODE_MFCR:
        {
            bytes = decode_form_reg2(emulator, pc, NULL, regs);
            // @TODO Check that control register is valid.
            check_registers(emulator, regs, 2);

            core->gprs[regs[0]] = core->crs[regs[1]];

            next_pc = pc + bytes;
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
            bytes = decode_form_reg3(emulator, pc, NULL, regs);
            check_registers(emulator, regs, 3);

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
        } break;
        case OPCODE_JMP:
        case OPCODE_JMP1:
        case OPCODE_JMP2:
        {
            bytes = decode_form_pc(emulator, pc, OPCODE_JMP, NULL, &relative);

            next_pc = pc + bytes + relative;
        } break;
        case OPCODE_CALL:
        case OPCODE_CALL1:
        case OPCODE_CALL2:
        {
            bytes = decode_form_pc(emulator, pc, OPCODE_CALL, NULL, &relative);

            core->lr = pc + bytes;
            next_pc = pc + bytes + relative;
        } break;
        case OPCODE_RET:
        {
            next_pc = core->lr;
        } break;
        case OPCODE_NOP:
        {
            next_pc = pc + 1;
        } break;
        case OPCODE_PUSH:
        case OPCODE_POP:
        {
            bytes = decode_form_reg1(emulator, pc, NULL, regs);
            check_registers(emulator, regs, 1);
            
            if (opcode == OPCODE_PUSH) {
                core->sp -= REGISTER_BYTESIZE();
            }
            switch (emulator->coreState.mode) {
                case MODE_16: WRITE16(core->sp, core->gprs[regs[0]]); break;
                case MODE_32: WRITE32(core->sp, core->gprs[regs[0]]); break;
                case MODE_64: WRITE64(core->sp, core->gprs[regs[0]]); break;
            }
            if (opcode == OPCODE_POP) {
                core->sp += REGISTER_BYTESIZE();
            }

            next_pc = pc + bytes;
        } break;
        case OPCODE_RDTICK:
        case OPCODE_RDTICK1:
        case OPCODE_RDTICK2:
        {
            uint64_t tickCounter = core->tickCounter;

            if (opcode == OPCODE_RDTICK) {
                bytes = decode_form_reg4(emulator, pc, NULL, regs);
                check_registers(emulator, regs, 4);

                core->gprs[regs[0]] = (tickCounter >>  0) & 0xFFFF;
                core->gprs[regs[1]] = (tickCounter >> 16) & 0xFFFF;
                core->gprs[regs[2]] = (tickCounter >> 32) & 0xFFFF;
                core->gprs[regs[3]] = (tickCounter >> 48) & 0xFFFF;
            } else if (opcode == OPCODE_RDTICK1) {
                bytes = decode_form_reg2(emulator, pc, NULL, regs);
                check_registers(emulator, regs, 2);

                core->gprs[regs[0]] = (tickCounter >>  0) & 0xFFFFFFFF;
                core->gprs[regs[1]] = (tickCounter >> 32) & 0xFFFFFFFF;
            } else if (opcode == OPCODE_RDTICK2) {
                bytes = decode_form_reg1(emulator, pc, NULL, regs);
                check_registers(emulator, regs, 1);
                
                core->gprs[regs[0]] = tickCounter;
            }

            next_pc = pc + bytes;
        } break;
        case OPCODE_JZ:
        case OPCODE_JNZ:
        {
            bytes = decode_form_jmp_reg1(emulator, pc, NULL, regs, &relative);
            check_registers(emulator, regs, 1);

            uint64_t result;
            if (core->mode == MODE_16) {
                result = (uint16_t)core->gprs[regs[0]];
            } else if (core->mode == MODE_32) {
                result = (uint32_t)core->gprs[regs[0]];
            } else if (core->mode == MODE_64) {
                result = (uint64_t)core->gprs[regs[0]];
            }

            if ((result == 0) == (opcode == OPCODE_JNZ) ) {
                next_pc = pc + bytes + relative;
            } else {
                next_pc = pc + bytes;
            }

        } break;
        case OPCODE_JCOND:
        {
            ConditionKind cond;
            bytes = decode_form_jmp_reg2(emulator, pc, NULL, &cond, regs, &relative);
            check_registers(emulator, regs, 2);

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
            bytes = decode_form_memory(emulator, pc, NULL, regs, &addressingForm, &displacement);
            check_registers(emulator, regs, 3);

            uint64_t result = 0;
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
                    address = core->gprs[regs[1]] + displacement;
                break;
                case ADDRESSING_REG2_DISP8:
                case ADDRESSING_REG2_DISP16:
                case ADDRESSING_REG2_DISP32:
                    address = core->gprs[regs[1]] + core->gprs[regs[2]] + displacement;
                break;
                case ADDRESSING_PC_DISP32:
                    address = pc + bytes + displacement;
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

            if (opcode >= OPCODE_LEA && OPCODE_LEA <= OPCODE_LDQ) {
                core->gprs[regs[0]] = result;
            }

            next_pc = pc + bytes;
        } break;
        
        case OPCODE_SYSCALL:
        {
            INCOMPLETE_INST("TODO implement syscall\n");
        } break;
        case OPCODE_VRET:
        {
            INCOMPLETE_INST("TODO implement vret\n");
        } break;
        case OPCODE_DBG:
        {
            INCOMPLETE_INST("TODO implement dbg\n");
        } break;
        case OPCODE_EINT:
        {
            INCOMPLETE_INST("TODO implement eint\n");
        } break;
        case OPCODE_DINT:
        {
            INCOMPLETE_INST("TODO implement dint\n");
        } break;
        case OPCODE_SLOW:
        {
            INCOMPLETE_INST("TODO implement slow\n");
        } break;
        case OPCODE_WFI:
        {
            INCOMPLETE_INST("TODO implement wfi\n");
        } break;
        case OPCODE_TLBFLUSH:
        {
            INCOMPLETE_INST("TODO implement tlbflush\n");
        } break;
        case OPCODE_CPUFEAT:
        {
            INCOMPLETE_INST("TODO implement cpufeat\n");
        } break;
        case OPCODE_VMSTART:
        {
            INCOMPLETE_INST("TODO implement vmstart\n");
        } break;
        default: {
            fprintf(stderr, "Exception ILLEGAL instruction at 0x%zx. Unknown opcode %d\n", pc, opcode);
            fault(emulator);
        }
    }

    if (emulator->running) {
        if (core->pc == next_pc) {
            fprintf(stderr, "WARNING: Executing same instruction %zx?\n", next_pc);
        }
        core->pc = next_pc;
    }

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
    printf("Core entry: %zu\n", config->core_entry);
    uint8_t* rom = config->rom;

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
    if ((i+1) % stride != 0) {
        printf("\n");
    }
}

