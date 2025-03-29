#include "lwmcore/emulator.h"


//#######################
//   PUBLIC FUNCTIONS
//#######################

bool emulate(Bin bin, EmulatorInfo* info) {
    // We expect info to be fully zeroed

    info->cpu.stack_max = 0x10000;
    info->cpu.stack = malloc(info->cpu.stack_max);
    memset(info->cpu.stack, 0, info->cpu.stack_max);

    bool running = true;
    while(running) {
        int pc = info->cpu.registers[REG_PC];

        // fetch
        u16 word = *(u16*)&bin.ptr[pc];
        info->cpu.registers[REG_PC] += 2;

        // decode
        int opcode = (word >> 8);
        int reg0 = 0;
        int reg1 = 0;
        int imm = 0;
        if((opcode & 0x80) == 0) {
            opcode = opcode >> 4;
            if(opcode == INST_CALL || opcode == INST_JMP) {
                imm = word & 0x7FF;
                if (word & 0x800)
                    imm = imm | 0xFFFF8000;
            } else {
                reg0 = (word >> 8) & 0xF;
                imm = (i8)(word & 0xFF);
            }
        } else if((opcode & 0x80) == 1) {
            opcode = opcode >> 3;
            reg0 = (word >> 8) & 0x7;
            imm = (i8)(word & 0xFF);
        } else {
            // extended
            reg0 = (word >> 4) & 0xF;
            if(opcode == INST_SHL || opcode == INST_SHR) {
                imm = (word) & 0xF;
            } else {
                reg1 = (word) & 0xF;
            }
        }

        // execute
        switch(opcode) {
            case INST_LI: {
                info->cpu.registers[reg0] = (info->cpu.registers[reg0] & 0xF0) | imm;
            } break;
            case INST_ADD: {
                info->cpu.registers[reg0] = info->cpu.registers[reg0] + info->cpu.registers[reg1];
            } break;
            case INST_SUB: {
                info->cpu.registers[reg0] = info->cpu.registers[reg0] - info->cpu.registers[reg1];
            } break;
            case INST_AND: {
                info->cpu.registers[reg0] = info->cpu.registers[reg0] & info->cpu.registers[reg1];
            } break;
            case INST_OR: {
                info->cpu.registers[reg0] = info->cpu.registers[reg0] | info->cpu.registers[reg1];
            } break;
            case INST_XOR: {
                info->cpu.registers[reg0] = info->cpu.registers[reg0] ^ info->cpu.registers[reg1];
            } break;
            case INST_CMP: {
                Assert(false);
            } break;
            case INST_SHL: {
                info->cpu.registers[reg0] = info->cpu.registers[reg0] << imm;
            } break;
            case INST_SHR: {
                info->cpu.registers[reg0] = info->cpu.registers[reg0] >> imm;
            } break;
            case INST_JMP: {
                info->cpu.registers[REG_PC] += imm;
            } break;
            case INST_CALL: {
                info->cpu.registers[REG_SP] += 2;
                *(u16*)&info->cpu.stack[info->cpu.registers[REG_SP]] = info->cpu.registers[REG_PC];
                info->cpu.registers[REG_PC] += imm;
            } break;
            case INST_RET: {
                int pc = *(u16*)&info->cpu.stack[info->cpu.registers[REG_SP]];
                u16 prev = info->cpu.registers[REG_SP];
                info->cpu.registers[REG_SP] -= 2;
                info->cpu.registers[REG_PC] = pc;

                if(prev < info->cpu.registers[REG_SP]) {
                    // stack wrapped around, we're done
                    running = false;
                    break;
                }
            } break;
            default: {
                string name = get_opcode_name(opcode);
                printf("Implement %s\n", name.ptr);
                Assert(false);
            };
        }
    }

    return true;
}

char tohex(int x) {
    if(x >= 0 && x <= 9)
        return '0' + x;
    if(x >= 10 && x <= 15)
        return 'A' + x - 10;
}

void dump(Bin bin) {
    for(int i=0;i<bin.len;i+=2) {
        u16 word = *(u16*)(&bin.ptr[i]);
        printf(" 0x%x: ", i);
        char c0 = tohex((word>>0)&0xF);
        char c1 = tohex((word>>4)&0xF);
        char c2 = tohex((word>>8)&0xF);
        char c3 = tohex((word>>12)&0xF);
        printf("%c%c %c%c ", c3, c2, c1, c0);

        int opcode = (word >> 8);
        if((opcode & 0x80) == 0) {
            opcode = opcode >> 4;
        } else if((opcode & 0x80) == 1) {
            opcode = opcode >> 3;
        }
        string name = get_opcode_name(opcode);
        printf("%s", name.ptr);

        printf("\n");
    }
}


//#######################
//   PRIVATE FUNCTIONS
//#######################

