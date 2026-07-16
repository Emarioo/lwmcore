


#include "lwm/emulator.h"
#include "lwm/isa.h"

#include "lwm/util.h"
#include "lwm/parser_util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>



#define PAGING_ENABLED() (core->crstatus & CRSTATUS_PAGING)
#define INTERRUPT_ENABLED() (core->crstatus & CRSTATUS_INTERRUPT)
#define USER_ENABLED() (core->crstatus & CRSTATUS_USER)

#define  READ8(address) read_exec_address8(core, address)
#define READ16(address) read_exec_address16(core, address)
#define READ32(address) read_exec_address32(core, address)
#define READ64(address) read_exec_address64(core, address)


#define BITMASK(WORD, BL, BH) ( ((WORD) >> BL) & ( ((uint64_t)1 << ((1+BH)-BL)) - 1 ) )

#define REGISTER_BYTESIZE() (2 << core->mode)

void hard_fault(CoreState* core) {
    for (int i=0;i<core->emulator->platformConfig->core_count;i++) {
        core->emulator->cores[i].running = false;
    }
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


typedef enum {
    MMU_READ,
    MMU_WRITE,
    MMU_READ_EXEC,
} MMUOperation;

void mmu_check_flags(CoreState* core, MMUOperation operation, uintptr_t virt_address, int flags) {

    #define SET_CAUSE(ACCESS_FLAG) do { core->crfault = virt_address; core->crcause = ACCESS_FLAG; } while (0)

    int mmuOpCause = 0;
    switch (operation) {
        case MMU_READ: {
            mmuOpCause = CRCAUSE_READ;
        } break;
        case MMU_WRITE: {
            mmuOpCause = CRCAUSE_WRITE;
        } break;
        case MMU_READ_EXEC: {
            mmuOpCause = CRCAUSE_EXECUTE;
        } break;
    }

    if (!(flags & PAGE_BIT_PRESENT)) {
        SET_CAUSE(mmuOpCause);
        emulator_trigger_exception(core, VECTOR_PAGE_FAULT);
    }
    mmuOpCause |= CRCAUSE_PRESENT;

    if (USER_ENABLED() && !(flags & PAGE_BIT_USER)) {
        SET_CAUSE(CRCAUSE_USER | mmuOpCause);
        emulator_trigger_exception(core, VECTOR_PAGE_FAULT);
    }

    switch (operation) {
        case MMU_READ: {
            if (!(flags & PAGE_BIT_READ)) {
                SET_CAUSE(mmuOpCause);
                emulator_trigger_exception(core, VECTOR_PAGE_FAULT);
            }
        } break;
        case MMU_WRITE: {
            if (!(flags & PAGE_BIT_WRITE)) {
                SET_CAUSE(mmuOpCause);
                emulator_trigger_exception(core, VECTOR_PAGE_FAULT);
            }
        } break;
        case MMU_READ_EXEC: {
            if (!(flags & PAGE_BIT_EXECUTE)) {
                SET_CAUSE(mmuOpCause);
                emulator_trigger_exception(core, VECTOR_PAGE_FAULT);
            }
        } break;
    }
}

void mmu_operation(CoreState* core, MMUOperation operation, uint64_t in_address, uint64_t size, void* data, bool physical) {
    EmulatorContext* emulator = core->emulator;

    if (operation != MMU_READ_EXEC && 0 != ((size-1) & in_address)) {
        // @TODO Misaligned access (crfault/crcause) is not documented.
        //   CRCAUSE should provide the access size, read/write MMU operation.
        core->crfault = in_address;
        core->crcause = size;
        printf("MISALIGNED at 0x%zx, 0x%zx, %zd bytes\n", core->pc, in_address, size);
        emulator_trigger_exception(core, VECTOR_MISALIGNED_ACCESS);
    }


    uintptr_t phys_address;
    if (physical || !PAGING_ENABLED()) {
        phys_address = in_address;
    } else {
        const uintptr_t virt_address = in_address;
        uint32_t flags;
        uint32_t index;
        uintptr_t pageDirectory = core->crpt;
        
        uintptr_t offset = virt_address & 0xFFF;
        if (core->mode == MODE_16) {
            uint32_t entry;
            
            index = (virt_address >> 12) & 0x3FF;
            mmu_operation(core, MMU_READ, pageDirectory + index * 4, 4, &entry, true);
            flags = entry & 0xFFF;
            mmu_check_flags(core, operation, virt_address, flags);
            
            phys_address = entry & 0x3FF000; // 22-bit address space, lower 12 bits unused
            phys_address += offset;
            
        } else if (core->mode == MODE_32) {
            uint32_t entry;

            // @TODO Implement huge bit
            
            index = (virt_address >> 22) & 0x3FF;
            mmu_operation(core, MMU_READ, pageDirectory + index * 4, 4, &entry, true);
            flags = entry & 0xFFF;
            mmu_check_flags(core, operation, virt_address, flags);
            pageDirectory = entry & 0xFFFFF000;
            
            index = (virt_address >> 12) & 0x3FF;
            mmu_operation(core, MMU_READ, pageDirectory + index * 4, 4, &entry, true);
            flags = entry & 0xFFF;
            mmu_check_flags(core, operation, virt_address, flags);
            
            phys_address = entry & 0xFFFFF000; // 32-bit address space, lower 12 bits unused
            phys_address += offset;

        } else if (core->mode == MODE_64) {
            uint64_t entry;
            
            // @TODO Implement huge bit

            index = (virt_address >> 39) & 0x1FF;
            mmu_operation(core, MMU_READ, pageDirectory + index * 8, 8, &entry, true);
            flags = entry & 0xFFF;
            mmu_check_flags(core, operation, virt_address, flags);
            pageDirectory = entry & 0xFFFFFFFFF000;

            index = (virt_address >> 30) & 0x1FF;
            mmu_operation(core, MMU_READ, pageDirectory + index * 8, 8, &entry, true);
            flags = entry & 0xFFF;
            mmu_check_flags(core, operation, virt_address, flags);
            pageDirectory = entry & 0xFFFFFFFFF000;
            
            index = (virt_address >> 21) & 0x1FF;
            mmu_operation(core, MMU_READ, pageDirectory + index * 8, 8, &entry, true);
            flags = entry & 0xFFF;
            mmu_check_flags(core, operation, virt_address, flags);
            pageDirectory = entry & 0xFFFFFFFFF000;

            index = (virt_address >> 12) & 0x1FF;
            mmu_operation(core, MMU_READ, pageDirectory + index * 8, 8, &entry, true);
            flags = entry & 0xFFF;
            mmu_check_flags(core, operation, virt_address, flags);
            
            phys_address = entry & 0xFFFFFFFFF000; // 48-bit address space, lower 12 bits unused
            phys_address += offset;

        } else {
            Assert(false);
        }

    }

    for (int i=0;i<emulator->platformConfig->devices_len;i++) {
        HardwareDevice* dev = emulator->platformConfig->devices[i];
        switch (operation) {
            case MMU_READ:
            case MMU_READ_EXEC: {
                if (dev->mmio_read) {
                    bool res = dev->mmio_read(emulator, dev, phys_address, size, data);
                    if (res) {
                        return;
                    }
                }
            } break;
            case MMU_WRITE: {
                if (dev->mmio_write) {
                    bool res = dev->mmio_write(emulator, dev, phys_address, size, data);
                    if (res) {
                        return;
                    }
                }
            } break;
        }
    }

    if (phys_address > emulator->physicalMemory_size - size) {
        printf("BUS ERROR at 0x%zx, outside physical memory 0x%zx\n", phys_address, emulator->physicalMemory_size);
        emulator_trigger_exception(core, VECTOR_BUS_ERROR);
        return;
    }
    void* emu_address = emulator->physicalMemory + phys_address;
    switch (operation) {
        case MMU_READ:
        case MMU_READ_EXEC: {
            switch (size) {
                case 1:  *(uint8_t*)data =  *(uint8_t*)emu_address; break;
                case 2: *(uint16_t*)data = *(uint16_t*)emu_address; break;
                case 4: *(uint32_t*)data = *(uint32_t*)emu_address; break;
                case 8: *(uint64_t*)data = *(uint64_t*)emu_address; break;
                default: Assert(false);
            }
        } break;
        case MMU_WRITE: {
            switch (size) {
                case 1:  *(uint8_t*)emu_address =  *(uint8_t*)data; break;
                case 2: *(uint16_t*)emu_address = *(uint16_t*)data; break;
                case 4: *(uint32_t*)emu_address = *(uint32_t*)data; break;
                case 8: *(uint64_t*)emu_address = *(uint64_t*)data; break;
                default: Assert(false);
            }
        } break;
    }
}


static inline uint8_t read_address8(CoreState* core, uint64_t address) {
    uint8_t value;
    mmu_operation(core, MMU_READ, address, 1, &value, false);
    return value;
}
static inline uint16_t read_address16(CoreState* core, uint64_t address) {
    uint16_t value;
    mmu_operation(core, MMU_READ, address, 2, &value, false);
    return value;
}
static inline uint32_t read_address32(CoreState* core, uint64_t address) {
    uint32_t value;
    mmu_operation(core, MMU_READ, address, 4, &value, false);
    return value;
}
static inline uint64_t read_address64(CoreState* core, uint64_t address) {
    uint64_t value;
    mmu_operation(core, MMU_READ, address, 8, &value, false);
    return value;
}
static inline uint8_t read_exec_address8(CoreState* core, uint64_t address) {
    uint8_t value;
    mmu_operation(core, MMU_READ_EXEC, address, 1, &value, false);
    return value;
}
static inline uint16_t read_exec_address16(CoreState* core, uint64_t address) {
    uint16_t value;
    mmu_operation(core, MMU_READ_EXEC, address, 2, &value, false);
    return value;
}
static inline uint32_t read_exec_address32(CoreState* core, uint64_t address) {
    uint32_t value;
    mmu_operation(core, MMU_READ_EXEC, address, 4, &value, false);
    return value;
}
static inline uint64_t read_exec_address64(CoreState* core, uint64_t address) {
    uint64_t value;
    mmu_operation(core, MMU_READ_EXEC, address, 8, &value, false);
    return value;
}
static inline void write_address8(CoreState* core, uint64_t address, uint8_t value) {
    mmu_operation(core, MMU_WRITE, address, 1, &value, false);
}
static inline void write_address16(CoreState* core, uint64_t address, uint16_t value) {
    mmu_operation(core, MMU_WRITE, address, 2, &value, false);
}
static inline void write_address32(CoreState* core, uint64_t address, uint32_t value) {
    mmu_operation(core, MMU_WRITE, address, 4, &value, false);
}
static inline void write_address64(CoreState* core, uint64_t address, uint64_t value) {
    mmu_operation(core, MMU_WRITE, address, 8, &value, false);
}



int decode_form_reg1_imm(CoreState* core, uint64_t pc, uint32_t opcodeBase, uint32_t* opcode, uint8_t regs[1], uint64_t* immediate) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }
    regs[0] = READ8(pc + 1);

    int immediateSize = 1 << (tmp_opcode - opcodeBase);

    if (opcodeBase == OPCODE_LIS8) {
        if (immediateSize == 1)
            *immediate = (int64_t)(int8_t)READ8(pc + 2);
        else if (immediateSize == 2)
            *immediate = (int64_t)(int16_t)READ16(pc + 2);
        else if (immediateSize == 4)
            *immediate = (int64_t)(int32_t)READ32(pc + 2);
        else if (immediateSize == 8)
            *immediate = (int64_t)READ64(pc + 2);
    } else {
        if (immediateSize == 1)
            *immediate = (uint64_t)(uint8_t)READ8(pc + 2);
        else if (immediateSize == 2)
            *immediate = (uint64_t)(uint16_t)READ16(pc + 2);
        else if (immediateSize == 4)
            *immediate = (uint64_t)(uint32_t)READ32(pc + 2);
        else if (immediateSize == 8)
            *immediate = (uint64_t)READ64(pc + 2);
    }

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

int decode_form_memory(CoreState* core, uint64_t pc, uint32_t* opcode, uint8_t regs[4], AddressingForm* addressingForm, int64_t* displacement) {
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
    regs[3] = 0;

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    uint8_t word = (uint8_t)READ8(pc+1);
    
    int flags = BITMASK(word, 0, 2);
    regs[0]   = BITMASK(word, 3, 7);

    if (tmp_opcode == OPCODE_CAS) {
        regs[1] = READ8(pc+2);
    }
    
    int baseBytes = tmp_opcode == OPCODE_CAS ? 3 : 2;
    int memoryRegIndexBase = tmp_opcode == OPCODE_CAS ? 2 : 1;

    AddressingForm form = ADDRESSING_ENUM(flags, 0);
    *addressingForm = form;

    if (form == ADDRESSING_ABS16) {
        *displacement = (uint16_t)READ16(pc + baseBytes);
        return largest_encoding(tmp_opcode, form);

    } else if (form == ADDRESSING_ABS32) {
        *displacement = (uint32_t)READ32(pc + baseBytes);
        return largest_encoding(tmp_opcode, form);

    } else if (form == ADDRESSING_ABS64) {
        *displacement = (uint64_t)READ64(pc + baseBytes);
        return largest_encoding(tmp_opcode, form);

    } else if (form == ADDRESSING_REG1_DISP8) {

        uint8_t extraWord = (uint8_t)READ8(pc + baseBytes);
        int extraFlags = BITMASK(extraWord, 0, 2);
        regs[memoryRegIndexBase]   = BITMASK(extraWord, 3, 7);

        form = ADDRESSING_ENUM(flags, extraFlags);
        *addressingForm = form;

        if (form == ADDRESSING_REG1_DISP8) {
            *displacement = (int8_t)READ8(pc + baseBytes + 1);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_DISP16) {
            *displacement = (int16_t)READ16(pc + baseBytes + 1);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_DISP32) {
            *displacement = (int32_t)READ32(pc + baseBytes + 1);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_DISP64) {
            *displacement = (int64_t)READ64(pc + baseBytes + 1);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_PC_DISP8) {
            *displacement = (int8_t)READ8(pc + baseBytes + 1);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_PC_DISP16) {
            *displacement = (int16_t)READ16(pc + baseBytes + 1);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG1_PC_DISP32) {
            *displacement = (int32_t)READ32(pc + baseBytes + 1);
            return largest_encoding(tmp_opcode, form);
        } else {
            emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
        }

    } else if (form == ADDRESSING_REG2_DISP8) {
        
        uint16_t extraWord = (uint16_t)READ16(pc + baseBytes);
        int extraFlags = BITMASK(extraWord, 0, 2) | (BITMASK(extraWord, 8, 10) << 3);
        regs[memoryRegIndexBase]   = BITMASK(extraWord, 3, 7);
        regs[memoryRegIndexBase + 1]   = BITMASK(extraWord, 11, 15);
        
        form = ADDRESSING_ENUM(flags, extraFlags);
        *addressingForm = form;

        if (form == ADDRESSING_REG2_DISP8) {
            *displacement = (int8_t)READ8(pc + baseBytes + 2);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG2_DISP16) {
            *displacement = (int16_t)READ16(pc + baseBytes + 2);
            return largest_encoding(tmp_opcode, form);
        } else if (form == ADDRESSING_REG2_DISP32) {
            *displacement = (int32_t)READ32(pc + baseBytes + 2);
            return largest_encoding(tmp_opcode, form);
        } else {
            emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
        }

    } else if (form == ADDRESSING_PC_DISP8) {
        *displacement = (int8_t)READ8(pc + baseBytes);
        return largest_encoding(tmp_opcode, form);
        
    } else if (form == ADDRESSING_PC_DISP16) {
        *displacement = (int16_t)READ16(pc + baseBytes);
        return largest_encoding(tmp_opcode, form);
        
    } else if (form == ADDRESSING_PC_DISP32) {
        *displacement = (int32_t)READ32(pc + baseBytes);
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
        if (core->tickCounter == 0) {
            printf("CORE[%d] Not started\n", i);
            continue;
        }

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
            printf("TRIPPLE FAULT at 0x%zx\n", core->pc);
            hard_fault(core);
        }
        core->insideDoubleFault = true;
        emulator_enter_vector(core, VECTOR_DOUBLE_FAULT);
    } else {
        core->insideFault = true;
        emulator_enter_vector(core, vector);
    }
    if (core->emulator->platformConfig->verbose) {
        printf("EXCEPTION cpu=%d vector=%d pc=0x%zx fault=0x%zx cause=0x%zx\n", (int)core->crcpuid, vector, core->pc, core->crfault, core->crcause);
    }
    longjmp(core->loop_jmpbuf, 1);
}

void emulator_enter_vector(CoreState* core, int vector) {
    uint64_t ex_handler = 0;
    int entrySize = core->mode == MODE_64 ? 8 : 4;
    uint64_t eh_address = core->crvb + vector * entrySize;

    // @TODO If this fails then it's a triple fault. Currently on first fault this will be a double fault.
    //   Reason it should be tripple fault is that we can't enter the vector so CPU must shutdown.
    //   Which it will eventually.
    mmu_operation(core, MMU_READ, eh_address, entrySize, &ex_handler, true);
    // printf("vectorEntry=0x%zx handler=0x%zx fault=0x%zx cause=0x%zx\n", eh_address, ex_handler, core->crfault, core->crcause);
    
    core->crepc = core->pc;
    core->cresp = core->sp;
    core->crestatus = core->crstatus;
    core->pc = ex_handler;
    core->sp = core->crisp;

    core->crstatus &= ~(CRSTATUS_USER|CRSTATUS_INTERRUPT);
}

#define BITS_PER_PENDING_VECTOR_INDEX (8*sizeof(*core->pendingVectors))
#define GET_PENDING_VECTOR(ARR, VECTOR) \
    ((core->pendingVectors[VECTOR/BITS_PER_PENDING_VECTOR_INDEX] >> (VECTOR%BITS_PER_PENDING_VECTOR_INDEX)) & 1)
#define ENABLE_PENDING_VECTOR(ARR, VECTOR) \
    (core->pendingVectors[VECTOR/BITS_PER_PENDING_VECTOR_INDEX] |= (uint64_t)1 << (VECTOR%BITS_PER_PENDING_VECTOR_INDEX))
#define DISABLE_PENDING_VECTOR(ARR, VECTOR) \
    (core->pendingVectors[VECTOR/BITS_PER_PENDING_VECTOR_INDEX] &= ~((uint64_t)1 << (VECTOR%BITS_PER_PENDING_VECTOR_INDEX)))

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
    ENABLE_PENDING_VECTOR(core->pendingVectors, vector);
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

// state is the seed
uint32_t rand32(uint32_t* inout_state) {
    // Where did i get this from?
    uint32_t state = *inout_state;
    *inout_state = state * 747796405u + 2891336453u;
    state = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (state >> 22u) ^ state;
}


void emulator_start(PlatformConfig* config) {

    EmulatorContext _ctx = {0};
    EmulatorContext* emulator = &_ctx;

    if (config->core_count == 0) {
        config->core_count = 1;
    }

    emulator->platformConfig = config;
    emulator->physicalMemory_size = config->ram_size;
    emulator->physicalMemory = malloc(emulator->physicalMemory_size);
    memset(emulator->physicalMemory, 0xDF, emulator->physicalMemory_size);

    /* @TODO Do we want a feature to load multiple ROMs at different locations.
         For example load a kernel ROM, a user application ROM, an INITRD (initial ram disk).
         In platform config:
            roms = [
                { 0x0,    "kernel.rom" },
                { 0x2000, "initrd.rom" },
                { 0x6000, "app.rom" },
            ]
    */
    memcpy(emulator->physicalMemory, config->rom, config->rom_len);

    for (int i=0;i<config->devices_len;i++) {
        HardwareDevice* device = config->devices[i];
        device->init(emulator, device);
    }

    emulator->cores = malloc(sizeof(CoreState) * config->core_count);
    memset(emulator->cores, 0, sizeof(CoreState) * config->core_count);


    for (int i=0;i<config->core_count;i++) {
        CoreState* core = &emulator->cores[i];
        core->emulator = emulator;
        core->mode = config->core_mode;
    }

    emulator_boot_core(emulator, 0, emulator->platformConfig->core_entry);

    if (!config->quiet) {
        printf("Start emulator\n");
    }

    int currentCoreIndex = 0;
    int executedInstructions = 0;
    
    emulator->randomState = 0xA19F0B25; // Chosen at random, affects core scheduling. We use pseudo-randomness for deterministic random scheduling.

    #define INSTRUCTIONS_PER_CORE_TIME_SLICE 5

    int nextInstructionSlice = 5;
    uint64_t startTime = timestamp();
    uint64_t prevTime = timestamp();
    while (true) {
        CoreState* core = &emulator->cores[currentCoreIndex];

        int inactiveCores = 0;
        for (inactiveCores=0;inactiveCores<config->core_count;inactiveCores++) {
            if (emulator->cores[inactiveCores].running)
                break;
        }
        if (inactiveCores == config->core_count) {
            break;
        }

        if (emulator->platformConfig->core_count > 1) {
            while (!core->running || executedInstructions > nextInstructionSlice) {
                currentCoreIndex = (currentCoreIndex + 1) % emulator->platformConfig->core_count;
                core = &emulator->cores[currentCoreIndex];
                if (executedInstructions != 0) {
                    nextInstructionSlice = 1 + rand32(&emulator->randomState) % 17;
                }
                executedInstructions = 0;
            }
            executedInstructions++;
        }

        core->tickCounter++;

        for (int i=0;i<config->devices_len;i++) {
            HardwareDevice* device = config->devices[i];
            if (device->tick) {
                device->tick(emulator, device);
            }
        }
        if (core->tickCounter == core->crtimercmp) {
            ENABLE_PENDING_VECTOR(core->pendingVectors, VECTOR_TIMER_INTERRUPT);
        }

        bool canHandleInterrupt = (core->crstatus & CRSTATUS_INTERRUPT);
        if (canHandleInterrupt) {
            for (int vi=0;vi<MAX_VECTORS;vi++) {
                int pending = GET_PENDING_VECTOR(core->pendingVectors, vi);
                if (pending) {
                    if (vi == VECTOR_TIMER_INTERRUPT) {
                        if (config->verbose) {
                            printf("TIMER INTERRUPT at 0x%zx tc=%zu\n", core->pc, core->tickCounter);
                        }
                    } else {
                        if (config->verbose) {
                            printf("INTERRUPT at 0x%zx vector=%d\n", core->pc, vi);
                        }
                    }
                    emulator_enter_vector(core, vi);
                    DISABLE_PENDING_VECTOR(core->pendingVectors, vi);
                }
            }
        }
        
        int jmpResult = setjmp(core->loop_jmpbuf);
        if (jmpResult == 0) {
            emulator_step(emulator, currentCoreIndex);
        } else {
            // Exception/fault
            continue;
        }

        uint64_t nowTime = timestamp();
        uint64_t deltaTime_ns = timestamp_to_ns(nowTime - prevTime);
        prevTime = nowTime;
        core->executionTime_ns += deltaTime_ns;

        uint64_t ticks_per_sec = (core->tickCounter * 1000000000) / core->executionTime_ns;
        core->avgTickFrequency = ticks_per_sec;
    }

    // @TODO We need to sum up instructions over all cores.
    uint64_t endTime = timestamp();
    uint64_t ns = timestamp_to_ns(endTime - startTime);
    uint64_t steps = emulator->cores[0].instructionSteps;
    uint64_t steps_per_sec = (steps * 1000000000) / ns;
    
    if (!config->quiet) {
        printf("Stop emulator (%zu us, %zu steps, %zu steps/sec)\n", ns / 1000, steps, steps_per_sec);
        emulator_dump_state(emulator);
    }
}

#define INCOMPLETE_INST(...) do { printf(__VA_ARGS__); hard_fault(core); } while (0)

// #define LOG_INST(FMT, ...)
#define LOG_INST(FMT, ...) if (!emulator->platformConfig->quiet && emulator->platformConfig->verbose) { printf("0x%03zx: " FMT, pc __VA_OPT__(,) __VA_ARGS__); }
// #define LOG_INST(FMT, ...) printf("0x%03zx: " FMT, pc __VA_OPT__(,) __VA_ARGS__);

void emulator_step(EmulatorContext* emulator, int cpuid) {
    CoreState* core = &emulator->cores[cpuid];

    core->instructionSteps++;

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
        case OPCODE_LIS8:
        case OPCODE_LIS16:
        case OPCODE_LIS32:
        case OPCODE_LIS64:
        {
            int opcodeBase = opcode >= OPCODE_LIS8 ? OPCODE_LIS8 : OPCODE_LI8;
            bytes = decode_form_reg1_imm(core, pc, opcodeBase, NULL, regs, &immediate);
            check_registers(core, regs, 1);

            if (opcode == OPCODE_LI32 && core->mode < MODE_32) {
                // if (!emulator->platformConfig->quiet) {
                printf("Exception ILLEGAL instruction at 0x%zx. li32 not available in 16-bit mode\n", pc);
                // }
                emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
            }
            if (opcode == OPCODE_LI64 && core->mode < MODE_64) {
                // if (!emulator->platformConfig->quiet) {
                printf("Exception ILLEGAL instruction at 0x%zx. li64 not available in 16 or 32-bit mode\n", pc);
                // }
                emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
            }

            // Sign extension for LIS happens when decoding and returning uint64 immediate.

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

            char regname0[20];
            char regname1[20];

            LOG_INST("%s %s, %s=0x%zx\n", opcode_to_string(opcode),
                cr_to_string(regs[0], regname0),
                gpr_to_string(regs[1], regname1), core->gprs[regs[1]]);
        } break;
        case OPCODE_MFCR:
        {
            check_privilege(core);

            bytes = decode_form_reg2(core, pc, NULL, regs);
            // @TODO Check that control register is valid.
            check_registers(core, regs, 2);

            core->gprs[regs[0]] = core->crs[regs[1]];

            next_pc = pc + bytes;

            char regname0[20];
            char regname1[20];

            LOG_INST("%s %s, %s=0x%zx\n", opcode_to_string(opcode),
                gpr_to_string(regs[0], regname0),
                cr_to_string(regs[1], regname1), core->crs[regs[1]]);
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

            char regname0[20];
            char regname1[20];

            LOG_INST("%s %s=0x%zx, %s=0x%zx\n", opcode_to_string(opcode),
                gpr_to_string(regs[0], regname0), core->gprs[regs[0]],
                cr_to_string(regs[1], regname1), core->crs[regs[1]]);
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
            } else Assert(false);

            if (opcode == OPCODE_UDIV || opcode == OPCODE_UMOD) {
                if (right == 0) {
                    emulator_trigger_exception(core, VECTOR_DIVISION_BY_ZERO);
                }
            } else if (opcode == OPCODE_SDIV || opcode == OPCODE_SMOD) {
                if (rightSigned == 0) {
                    emulator_trigger_exception(core, VECTOR_DIVISION_BY_ZERO);
                }
            }

            switch (opcode) {
                case OPCODE_ADD:  result = left + right; break;
                case OPCODE_SUB:  result = left - right; break;
                case OPCODE_UMUL: result = left * right; break;
                case OPCODE_UDIV: result = left / right; break;
                case OPCODE_UMOD: result = left % right; break;
                case OPCODE_SMUL: result = leftSigned * rightSigned; break;
                case OPCODE_SDIV: result = leftSigned / rightSigned; break;
                // @TODO We assume rightSigned is positive. What do we do if it isn't?
                //    A fault like MOD_BY_NEGATIVE like how there is DIV_BY_ZERO? We would combine into MATH_FAULT and then provide info in error code.
                case OPCODE_SMOD: result = ((leftSigned % rightSigned) + rightSigned) % rightSigned ; break;
                case OPCODE_AND:  result = left & right; break;
                case OPCODE_OR:   result = left | right; break;
                case OPCODE_XOR:  result = left ^ right; break;
                case OPCODE_SHL:  result = left << right; break;
                case OPCODE_SHR:  result = left >> right; break;
                default: Assert(false);
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

            if (opcode == OPCODE_PUSH) {
                core->sp -= REGISTER_BYTESIZE();
                switch (core->mode) {
                    case MODE_16: write_address16(core, core->sp, core->gprs[regs[0]]); break;
                    case MODE_32: write_address32(core, core->sp, core->gprs[regs[0]]); break;
                    case MODE_64: write_address64(core, core->sp, core->gprs[regs[0]]); break;
                }
            }
            if (opcode == OPCODE_POP) {
                switch (core->mode) {
                    case MODE_16: core->gprs[regs[0]] = read_address16(core, core->sp); break;
                    case MODE_32: core->gprs[regs[0]] = read_address32(core, core->sp); break;
                    case MODE_64: core->gprs[regs[0]] = read_address64(core, core->sp); break;
                }
                core->sp += REGISTER_BYTESIZE();
            }

            next_pc = pc + bytes;
            LOG_INST("%s\n", opcode_to_string(opcode));
        } break;
        case OPCODE_SAVE:
        case OPCODE_RESTORE:
        {
            check_privilege(core);

            bytes = decode_form_reg1_imm(core, pc, opcode, NULL, regs, &immediate);
            check_registers(core, regs, 1);

            int regCount = core->mode == MODE_16 ? 16 : 32;
            int regSize  = REGISTER_BYTESIZE();
            uintptr_t address = core->gprs[regs[0]];

            // @TODO Immediate tells us which registers to save. For now we just always same GPRS.
            //   We don't have floating point and control registers needs special care when saving/restoring.

            for (int i=0;i<regCount;i++) {
                if (opcode == OPCODE_SAVE) {
                    if (i == regs[0])
                        continue;
                    if (i == LWM_REGNR_SP) {
                        mmu_operation(core, MMU_WRITE, address + i*regSize, regSize, &core->crs[LWM_REGNR_CRESP], false);
                    } else {
                        mmu_operation(core, MMU_WRITE, address + i*regSize, regSize, &core->gprs[i], false);
                    }
                } else {
                    if (i == LWM_REGNR_SP) {
                        mmu_operation(core, MMU_READ, address + i*regSize, regSize, &core->crs[LWM_REGNR_CRESP], false);
                    } else {
                        mmu_operation(core, MMU_READ, address + i*regSize, regSize, &core->gprs[i], false);
                    }
                }
            }

            next_pc = pc + bytes;

            LOG_INST("%s r%d, 0x%zx\n", opcode_to_string(opcode), regs[0], immediate);
        } break;
        case OPCODE_RDTICK:
        case OPCODE_RDTICK1:
        case OPCODE_RDTICK2:
        {
            uint64_t tickCounter = core->tickCounter;

            if (opcode == OPCODE_RDTICK2) {
                bytes = decode_form_reg4(core, pc, NULL, regs);
                check_registers(core, regs, 4);
                
                core->gprs[regs[0]] = (tickCounter >>  0) & 0xFFFF;
                core->gprs[regs[1]] = (tickCounter >> 16) & 0xFFFF;
                core->gprs[regs[2]] = (tickCounter >> 32) & 0xFFFF;
                core->gprs[regs[3]] = (tickCounter >> 48) & 0xFFFF;
            } else if (opcode == OPCODE_RDTICK1) {
                bytes = decode_form_reg2(core, pc, NULL, regs);
                check_registers(core, regs, 2);

                if (core->mode == MODE_16) {
                    core->gprs[regs[0]] = (tickCounter >>  0) & 0xFFFF;
                    core->gprs[regs[1]] = (tickCounter >> 16) & 0xFFFF;
                } else {
                    core->gprs[regs[0]] = (tickCounter >>  0) & 0xFFFFFFFF;
                    core->gprs[regs[1]] = (tickCounter >> 32) & 0xFFFFFFFF;
                }
            } else if (opcode == OPCODE_RDTICK) {
                bytes = decode_form_reg1(core, pc, NULL, regs);
                check_registers(core, regs, 1);
                
                if (core->mode == MODE_16) {
                    core->gprs[regs[0]] = (tickCounter >>  0) & 0xFFFF;
                } else if (core->mode == MODE_32) {
                    core->gprs[regs[0]] = (tickCounter >>  0) & 0xFFFFFFFF;
                } else {
                    core->gprs[regs[0]] = tickCounter;
                }
            } else Assert(false);

            next_pc = pc + bytes;

            LOG_INST("rdtick %zu, tc=%zu\n", core->gprs[regs[0]], tickCounter);
        } break;
        case OPCODE_ADVTIMER:
        case OPCODE_ADVTIMER1:
        case OPCODE_ADVTIMER2:
        {
            check_privilege(core);

            uint64_t delta;
            if (opcode == OPCODE_ADVTIMER2) {
                bytes = decode_form_reg4(core, pc, NULL, regs);
                check_registers(core, regs, 4);

                delta = ((core->gprs[regs[0]] & 0xFFFF) <<  0) |
                        ((core->gprs[regs[1]] & 0xFFFF) << 16) |
                        ((core->gprs[regs[2]] & 0xFFFF) << 32) |
                        ((core->gprs[regs[3]] & 0xFFFF) << 48);

            } else if (opcode == OPCODE_ADVTIMER1) {
                bytes = decode_form_reg2(core, pc, NULL, regs);
                check_registers(core, regs, 2);
                
                if (core->mode == MODE_16) {
                    delta = ((core->gprs[regs[0]] & 0xFFFF) <<  0) |
                            ((core->gprs[regs[1]] & 0xFFFF) << 16);
                } else {
                    delta = ((core->gprs[regs[0]] & 0xFFFFFFFF) <<  0) |
                            ((core->gprs[regs[1]] & 0xFFFFFFFF) << 32);
                }
            } else if (opcode == OPCODE_ADVTIMER) {
                bytes = decode_form_reg1(core, pc, NULL, regs);
                check_registers(core, regs, 1);

                if (core->mode == MODE_16) {
                    delta = ((core->gprs[regs[0]] & 0xFFFF) <<  0);
                } else if (core->mode == MODE_32) {
                    delta = ((core->gprs[regs[0]] & 0xFFFFFFFF) <<  0);
                } else {
                    delta = core->gprs[regs[0]];
                }
            } else Assert(false);
            core->crtimercmp = core->tickCounter + delta;

            next_pc = pc + bytes;

            LOG_INST("advtimer delta=%zu, timercmp=%zu\n", delta, core->crtimercmp);
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
            } else Assert(false);

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
            } else Assert(false);

            switch (cond) {
                case COND_EQ: result = left == right; break;
                case COND_NE: result = left != right; break;
                case COND_LT: result = leftSigned < rightSigned; break;
                case COND_LE: result = leftSigned <= rightSigned; break;
                case COND_GT: result = leftSigned > rightSigned; break;
                case COND_GE: result = leftSigned >= rightSigned; break;
                case COND_B:  result = left < right; break;
                case COND_BE: result = left >= right; break;
                case COND_A:  result = left > right; break;
                case COND_AE: result = left >= right; break;
                default: Assert(false);
            }

            if (result) {
                next_pc = pc + bytes + relative;
            } else {
                next_pc = pc + bytes;
            }
            LOG_INST("%s %d %d\n", opcode_to_string(opcode), cond, result);
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
        case OPCODE_XADD:
        case OPCODE_CAS:
        {
            AddressingForm addressingForm;
            int64_t displacement;
            bytes = decode_form_memory(core, pc, NULL, regs, &addressingForm, &displacement);
            check_registers(core, regs, 4);

            if ((opcode == OPCODE_LDL || opcode == OPCODE_LDLS || opcode == OPCODE_STL)
                && core->mode < MODE_32) {
                // if (!emulator->platformConfig->quiet) {
                printf("Exception ILLEGAL instruction at 0x%zx. store/load long not available in 16-bit mode\n", pc);
                // }
                emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
            }
            if ((opcode == OPCODE_LDQ || opcode == OPCODE_STQ) && core->mode < MODE_64) {
                // if (!emulator->platformConfig->quiet) {
                printf("Exception ILLEGAL instruction at 0x%zx. store/load quad not available in 16 or 32-bit mode\n", pc);
                // }
                emulator_trigger_exception(core, VECTOR_ILLEGAL_INSTRUCTION);
            }

            // @TODO Certain addressing forms such as 64-bit absolute should not be allowed in 16-bit mode.
            //   We need to decide which ones and put it into ISA. We need to put info that li64 not available in 16-bit mode too.

            uint64_t result;
            uint64_t address;

            int memoryRegIndexBase = opcode == OPCODE_CAS ? 2 : 1;

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
                    address = core->gprs[regs[memoryRegIndexBase]] + displacement;
                break;
                case ADDRESSING_REG2_DISP8:
                case ADDRESSING_REG2_DISP16:
                case ADDRESSING_REG2_DISP32:
                case ADDRESSING_REG2_DISP64:
                    address = core->gprs[regs[memoryRegIndexBase]] + core->gprs[regs[memoryRegIndexBase+1]] + displacement;
                break;
                case ADDRESSING_PC_DISP8:
                case ADDRESSING_PC_DISP16:
                case ADDRESSING_PC_DISP32:
                    address = pc + bytes + displacement;
                break;
                case ADDRESSING_REG1_PC_DISP8:
                case ADDRESSING_REG1_PC_DISP16:
                case ADDRESSING_REG1_PC_DISP32:
                    address = core->gprs[regs[memoryRegIndexBase]] + pc + bytes + displacement;
                break;
                default: Assert(false);
            }

            uint64_t value = core->gprs[regs[0]]; // Here for debugging. Only needed for stores.
            
            switch (opcode) {
                case OPCODE_LEA:  result = address; break;
                case OPCODE_LDB:  result =          (uint8_t)read_address8(core, address); break;
                case OPCODE_LDBS: result =  (int64_t)(int8_t)read_address8(core, address); break;
                case OPCODE_LDH:  result =         (uint16_t)read_address16(core, address); break;
                case OPCODE_LDHS: result = (int64_t)(int16_t)read_address16(core, address); break;
                case OPCODE_LDL:  result =         (uint32_t)read_address32(core, address); break;
                case OPCODE_LDLS: result = (int64_t)(int32_t)read_address32(core, address); break;
                case OPCODE_LDQ:  result =         (uint64_t)read_address64(core, address); break;
                case OPCODE_STB:  write_address8(core, address,  (uint8_t)value); break;
                case OPCODE_STH:  write_address16(core, address, (uint16_t)value); break;
                case OPCODE_STL:  write_address32(core, address, (uint32_t)value); break;
                case OPCODE_STQ:  write_address64(core, address, (uint64_t)value); break;
                case OPCODE_XADD:  {
                    uint64_t temp = 0;
                    mmu_operation(core, MMU_READ, address, REGISTER_BYTESIZE(), &temp, false);
                    temp += value;
                    mmu_operation(core, MMU_WRITE, address, REGISTER_BYTESIZE(), &temp, false);
                } break;
                case OPCODE_CAS:  {
                    // @TODO If we put each core on a thread then we need to use lock mechanism from host CPU.
                    uint64_t old = 0;
                    mmu_operation(core, MMU_READ, address, REGISTER_BYTESIZE(), &old, false);
                    uint64_t expected;
                    switch (core->mode) {
                        case MODE_16:
                            expected = (uint16_t)core->gprs[regs[0]];
                            break;
                        case MODE_32:
                            expected = (uint32_t)core->gprs[regs[0]];
                            break;
                        case MODE_64:
                            expected = (uint64_t)core->gprs[regs[0]];
                            break;
                        default: Assert(false);
                    }
                    if (old == expected) {
                        uint64_t newValue = core->gprs[regs[1]];
                        mmu_operation(core, MMU_WRITE, address, REGISTER_BYTESIZE(), &newValue, false);
                    }
                    core->gprs[regs[1]] = old;  // rNew = old
                } break;
                default: Assert(false);
            }

            next_pc = pc + bytes;

            if (opcode >= OPCODE_LEA && opcode <= OPCODE_LDQ) {
                core->gprs[regs[0]] = result;
                LOG_INST("%s r%d = %zd, addr=0x%zx\n", opcode_to_string(opcode), regs[0], result, address);
            } else if (opcode == OPCODE_CAS) {
                LOG_INST("%s r%d = %zd, addr=0x%zx\n", opcode_to_string(opcode), regs[0], value, address);
            } else {
                LOG_INST("%s r%d = %zd, addr=0x%zx\n", opcode_to_string(opcode), regs[0], value, address);
            }
        } break;
        case OPCODE_SYSCALL:
        {
            core->pc = pc + 1; // We must set PC in here since trigger exception
                               // moves it to CREPC
            LOG_INST("%s\n", opcode_to_string(opcode));
            emulator_trigger_exception(core, VECTOR_SYSCALL);
        } break;
        case OPCODE_VRET:
        {
            check_privilege(core);

            // When entering user mode, paging and interrupts must be enabled
            if ((core->crestatus & CRSTATUS_USER) && (
                !(core->crestatus & CRSTATUS_PAGING) || !(core->crestatus & CRSTATUS_INTERRUPT)))
            {
                emulator_trigger_exception(core, VECTOR_PROTECTION_FAULT);
            }

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
            sleep_us(1000);
            
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
            bytes = decode_form_reg2(core, pc, NULL, regs);
            check_registers(core, regs, 2);

            FeatureID featureID = core->gprs[regs[1]];
            uint64_t result = 0;

            switch (featureID) {
                case CPUFEAT_REG_BYTES: {
                    result = REGISTER_BYTESIZE();
                } break;
                default: {
                    result = 0;
                } break;
            }

            core->gprs[regs[0]] = result;

            next_pc = pc + bytes;

            char regname0[20];
            char regname1[20];
            LOG_INST("%s %s=0x%zx, %s=%d\n", opcode_to_string(opcode), gpr_to_string(regs[0], regname0), result, gpr_to_string(regs[1], regname1), featureID);
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
    printf("Core count: %d\n", (int)config->core_count);
    printf("Core mode: ");
    switch (config->core_mode) {
        case MODE_16: printf(" 16-bit\n"); break;
        case MODE_32: printf(" 32-bit\n"); break;
        case MODE_64: printf(" 64-bit\n"); break;
        default:      printf(" %d\n", config->core_mode); break;
    }

    int stride = 4;
    
    printf("ROM %d bytes (0x%x)\n", config->rom_len, config->rom_len);

    bool skipping = false;
    for (int row = 0; row < config->rom_len; row += stride) {

        bool all_zero = true;
        int end = row + stride;
        if (end > config->rom_len)
            end = config->rom_len;

        for (int i = row; i < end; i++) {
            if (rom[i] != 0) {
                all_zero = false;
                break;
            }
        }
        
        if (all_zero && config->rom_len > 64) {
            if (!skipping) {
                printf("  ...\n");
                skipping = true;
            }
            continue;
        }

        skipping = false;

        printf(" 0x%x: ", row);

        for (int i = row; i < end; i++) {
            printf("%02X ", rom[i]);
        }

        printf("\n");
    }

}

#define ERROR_SRC_RET(HEAD, FORMAT, ...) SourceLocation loc = get_location(context, HEAD); error_src(loc, FORMAT, ##__VA_ARGS__); longjmp(context->jumpBuffer, 1)


bool parse_platform_config(const char* path, PlatformConfig* config) {
    bool res;
    char*  fileBuffer;
    size_t fileBuffer_len;
    res = readFile(path, (void**)&fileBuffer, &fileBuffer_len);
    if (!res) {
        return false;
    }

    ParserContext _context = {0};
    ParserContext* context = &_context;

    SourceSpan defaultSpan = {
        .file = path,
        .dst_start = 0,   
        .dst_end = fileBuffer_len,
        .src_start = 0,   
        .src_end = fileBuffer_len,
    };
    context->spans_len = 1;
    context->spans = &defaultSpan;
    
    context->path = path;
    context->text = fileBuffer;
    context->text_len = fileBuffer_len;

    int jmpResult = setjmp(context->jumpBuffer);
    if (jmpResult != 0) {
        return false;
    }

    config->core_count = 2;
    config->core_mode = MODE_16;
    config->core_entry = 0x0;
    config->devices_len = 0;
    config->devices = NULL;
    config->rom_path = NULL;
    config->rom = NULL;
    config->rom_len = 0;
    config->ram_size = 0x400000;

    int head = 0;
    int parsedChars;
    while (true) {

        parse_space(context, &head);
        
        if(parse_eof(context, head)) {
            break;
        }
        
        string keyname;
        int headAtKeyname = head;
        
        parsedChars = parse_name(context, &head, &keyname);
        if (!parsedChars) {
            ERROR_SRC_RET(head, "Expected a key name.\n");
        }
        
        parse_space(context, &head);
        
        char chr = get_char(context, head);
        if (chr != '=') {
            ERROR_SRC_RET(head, "Expected '='.\n");
        }
        head++;
        
        parse_space(context, &head);

        if (equal(keyname, "core_count")) {
            uint64_t value;
            parsedChars = parse_int(context, &head, &value);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a number.\n");
            }
            
            config->core_count = value;

        } else if (equal(keyname, "core_entry")) {
            uint64_t value;
            parsedChars = parse_int(context, &head, &value);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a number.\n");
            }
            
            config->core_entry = value;

        } else if (equal(keyname, "ram_size")) {
            uint64_t value;
            parsedChars = parse_int(context, &head, &value);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a number.\n");
            }

            int headAtSuffix = head;
            
            string suffix;
            parse_name(context, &head, &suffix);
            if (suffix.len == 0) {
                // no suffix
            } else if (equal(suffix, "KB")) {
                value *= 1024;
            } else if (equal(suffix, "MB")) {
                value *= 1024 * 1024;
            } else if (equal(suffix, "GB")) {
                value *= 1024 * 1024 * 1024;
            } else {
                ERROR_SRC_RET(headAtSuffix, "Unknown suffix '%s'. KB/MB/GB are valid.\n", suffix.ptr);
            }
            
            config->ram_size = value;

        } else if (equal(keyname, "cpu_family")) {
            int headAtValue = head;

            string family;
            parsedChars = parse_name(context, &head, &family);

            CoreMode mode;
            if (equal(family, "lwm16")) {
                mode = MODE_16;
            } else if (equal(family, "lwm32")) {
                mode = MODE_32;
            } else if (equal(family, "lwm64")) {
                mode = MODE_64;
            } else {
                ERROR_SRC_RET(headAtValue, "Unknown family '%s'. lwm16/lwm32/lwm64 are valid.\n", family.ptr);
            }

            config->core_mode = mode;

        } else if (equal(keyname, "cpu_features")) {

            ERROR_SRC_RET(head, "CPU features are not implemented.\n");
            
        } else if (equal(keyname, "devices")) {

            ERROR_SRC_RET(head, "Devices are not implemented in config file. They must be hardcoded in the emulator.\n");
            
        } else {

            ERROR_SRC_RET(headAtKeyname, "Unknown config option.\n");

        }
    }

    // @TODO Memory leak

    return true;
}

