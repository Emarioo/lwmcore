
#include "lwm/encoding.h"
#include "lwm/asm_types.h"


bool prepare_encode_inst(string mnemonic, Instruction* inst, int* minBytes, int* maxBytes) {

    #define CASE_OPCODE(STRING, OPCODE) if (equal(mnemonic, STRING)) { inst->opcode = OPCODE; }
    #define CASE_OPCODE2(STRING, OPCODE, SUB_OPCODE) if (equal(mnemonic, STRING)) { inst->opcode = OPCODE; inst->sub_opcode = SUB_OPCODE; }

    CASE_OPCODE("li", OPCODE_LI64)
    else CASE_OPCODE("lis", OPCODE_LIS64)
    else if (equal(mnemonic, "call")) {
        inst->opcode = OPCODE_CALL;
        Operand* labelOperand = &inst->operands[0];
        if (!labelOperand->label.len) {
            inst->opcode = OPCODE_CALL_REG;
        }
    }
    else if (equal(mnemonic, "jmp")) {
        inst->opcode = OPCODE_JMP;
        Operand* labelOperand = &inst->operands[0];
        if (!labelOperand->label.len) {
            inst->opcode = OPCODE_JMP_REG;
        }
    }
    else CASE_OPCODE("jz", OPCODE_JZ)
    else CASE_OPCODE("jnz", OPCODE_JNZ)
    else CASE_OPCODE2("jeq", OPCODE_JCOND, COND_EQ)
    else CASE_OPCODE2("jne", OPCODE_JCOND, COND_NE)
    else CASE_OPCODE2("jlt", OPCODE_JCOND, COND_LT)
    else CASE_OPCODE2("jle", OPCODE_JCOND, COND_LE)
    else CASE_OPCODE2("jgt", OPCODE_JCOND, COND_GT)
    else CASE_OPCODE2("jge", OPCODE_JCOND, COND_GE)
    else CASE_OPCODE2("jb",  OPCODE_JCOND, COND_B)
    else CASE_OPCODE2("jbe", OPCODE_JCOND, COND_BE)
    else CASE_OPCODE2("ja",  OPCODE_JCOND, COND_A)
    else CASE_OPCODE2("jae", OPCODE_JCOND, COND_AE)
    else CASE_OPCODE("not", OPCODE_NOT)
    else CASE_OPCODE("mfcr", OPCODE_MFCR) 
    else CASE_OPCODE("mtcr", OPCODE_MTCR) 
    else CASE_OPCODE("mscr", OPCODE_MSCR) 
    else CASE_OPCODE("cpufeat", OPCODE_CPUFEAT)
    else CASE_OPCODE("add", OPCODE_ADD) 
    else CASE_OPCODE("sub", OPCODE_SUB) 
    else CASE_OPCODE("umul", OPCODE_UMUL) 
    else CASE_OPCODE("smul", OPCODE_SMUL) 
    else CASE_OPCODE("udiv", OPCODE_UDIV) 
    else CASE_OPCODE("sdiv", OPCODE_SDIV) 
    else CASE_OPCODE("umod", OPCODE_UMOD) 
    else CASE_OPCODE("smod", OPCODE_SMOD) 
    else CASE_OPCODE("and", OPCODE_AND) 
    else CASE_OPCODE("or", OPCODE_OR) 
    else CASE_OPCODE("xor", OPCODE_XOR) 
    else CASE_OPCODE("shl", OPCODE_SHL) 
    else CASE_OPCODE("shr", OPCODE_SHR) 
    else CASE_OPCODE("ret", OPCODE_RET) 
    else CASE_OPCODE("syscall", OPCODE_SYSCALL) 
    else CASE_OPCODE("vret", OPCODE_VRET) 
    else CASE_OPCODE("dbg", OPCODE_DBG) 
    else CASE_OPCODE("eint", OPCODE_EINT) 
    else CASE_OPCODE("dint", OPCODE_DINT) 
    else CASE_OPCODE("slow", OPCODE_SLOW) 
    else CASE_OPCODE("wfi", OPCODE_WFI) 
    else CASE_OPCODE("nop", OPCODE_NOP) 
    else CASE_OPCODE("vmstart", OPCODE_VMSTART) 
    else CASE_OPCODE2("lea", OPCODE_LEA, MEMOP_LEA) 
    else CASE_OPCODE2("ldb", OPCODE_LDB, MEMOP_LDB) 
    else CASE_OPCODE2("ldbs", OPCODE_LDBS, MEMOP_LDBS) 
    else CASE_OPCODE2("ldh", OPCODE_LDH, MEMOP_LDH) 
    else CASE_OPCODE2("ldhs", OPCODE_LDHS, MEMOP_LDHS) 
    else CASE_OPCODE2("ldl", OPCODE_LDL, MEMOP_LDL) 
    else CASE_OPCODE2("ldls", OPCODE_LDLS, MEMOP_LDLS) 
    else CASE_OPCODE2("ldq", OPCODE_LDQ, MEMOP_LDQ) 
    else CASE_OPCODE2("stb", OPCODE_STB, MEMOP_STB) 
    else CASE_OPCODE2("sth", OPCODE_STH, MEMOP_STH) 
    else CASE_OPCODE2("stl", OPCODE_STL, MEMOP_STL) 
    else CASE_OPCODE2("stq", OPCODE_STQ, MEMOP_STQ) 
    else CASE_OPCODE2("xadd", OPCODE_XADD, MEMOP_XADD) 
    else CASE_OPCODE2("cas", OPCODE_CAS, MEMOP_CAS) 
    else CASE_OPCODE("push", OPCODE_PUSH) 
    else CASE_OPCODE("pop", OPCODE_POP) 
    else CASE_OPCODE("save", OPCODE_SAVE) 
    else CASE_OPCODE("restore", OPCODE_RESTORE)
    else CASE_OPCODE("tlbflush", OPCODE_TLBFLUSH) 
    else CASE_OPCODE("rdtick", OPCODE_RDTICK)
    else CASE_OPCODE("advtimer", OPCODE_ADVTIMER)
    else {
        return false;
    }

    *maxBytes = largest_encoding_ext(inst->opcode, inst->operands[1].form, minBytes);
    if (*minBytes == 0)
        *minBytes = *maxBytes;

    return true;
}


bool encode_inst(Builder* builder, Instruction* inst, char message[512]) {
    
    int message_max = 512;

    #define ERR_IMM_REG() \
        snprintf(message, message_max, "Immediate or register is not allowed.\n"); \
        return false;

    #define EMIT_REG3(op) \
        emit_##op(builder, inst->operands[0].regnum, inst->operands[1].regnum, inst->operands[2].regnum)
    #define EMIT_REG2(op) \
        emit_##op(builder, inst->operands[0].regnum, inst->operands[1].regnum)
    #define EMIT_REG1(op) \
        emit_##op(builder, inst->operands[0].regnum)

    switch ((OpcodeKind)inst->opcode) {
        case OPCODE_LI8:
        case OPCODE_LI16:
        case OPCODE_LI32:
        case OPCODE_LI64:
        {
            uint64_t imm = inst->operands[1].immediate;
            if (imm <= 0xFF) {
                emit_li8(builder, inst->operands[0].regnum, imm);
            } else if (imm <= 0xFFFF) {
                emit_li16(builder, inst->operands[0].regnum, imm);
            } else if (imm <= 0xFFFFFFFF) {
                emit_li32(builder, inst->operands[0].regnum, imm);
            } else {
                emit_li64(builder, inst->operands[0].regnum, imm);
            }
            // }
        } break;
        case OPCODE_LIS8:
        case OPCODE_LIS16:
        case OPCODE_LIS32:
        case OPCODE_LIS64:
        {
            // Signed
            int64_t imm = inst->operands[1].immediate;
            if (imm >= -0x80 && imm < 0x80) {
                emit_lis8(builder, inst->operands[0].regnum, imm);
            } else if (imm >= -0x8000 && imm < 0x8000) {
                emit_lis16(builder, inst->operands[0].regnum, imm);
            } else if (imm >= -0x80000000 && imm < 0x80000000) {
                emit_lis32(builder, inst->operands[0].regnum, imm);
            } else {
                emit_lis64(builder, inst->operands[0].regnum, imm);
            }
        } break;
        case OPCODE_CALL_REG: {
            emit_call_reg(builder, inst->operands[0].regnum);
        } break;
        case OPCODE_JMP_REG: {
            emit_call_reg(builder, inst->operands[0].regnum);
        } break;
        case OPCODE_CALL:
        case OPCODE_CALL1:
        case OPCODE_CALL2: {
            Operand* labelOperand = &inst->operands[0];
            if (!labelOperand->label.len) {
                ERR_IMM_REG();
            }
            LabelFixup* fixup = builder->create_fixup(builder->context, inst, labelOperand);
            if (fixup->reloc_size == 1) {
                emit_call8(builder, &fixup->rom_offset);
            } else if (fixup->reloc_size == 2) {
                emit_call16(builder, &fixup->rom_offset);
            } else {
                emit_call32(builder, &fixup->rom_offset);
            }
        } break;
        case OPCODE_JMP:
        case OPCODE_JMP1:
        case OPCODE_JMP2: {
            Operand* labelOperand = &inst->operands[0];
            if (!labelOperand->label.len) {
                ERR_IMM_REG();
            }
            LabelFixup* fixup = builder->create_fixup(builder->context, inst, labelOperand);
            if (fixup->reloc_size == 1) {
                emit_jmp8(builder, &fixup->rom_offset);
            } else if (fixup->reloc_size == 2) {
                emit_jmp16(builder, &fixup->rom_offset);
            } else {
                emit_jmp32(builder, &fixup->rom_offset);
            }
        } break;
        case OPCODE_JZ: {
            Operand* labelOperand = &inst->operands[1];
            if (!labelOperand->label.len) {
                ERR_IMM_REG();
            }
            LabelFixup* fixup = builder->create_fixup(builder->context, inst, labelOperand);
            
            if (fixup->reloc_size == 1) {
                emit_jz8(builder, inst->operands[0].regnum, &fixup->rom_offset);
            } else if (fixup->reloc_size == 2) {
                emit_jz16(builder, inst->operands[0].regnum, &fixup->rom_offset);
            } else {
                emit_jz32(builder, inst->operands[0].regnum, &fixup->rom_offset);
            }
        } break;
        case OPCODE_JNZ: {
            Operand* labelOperand = &inst->operands[1];
            if (!labelOperand->label.len) {
                ERR_IMM_REG();
            }
            LabelFixup* fixup = builder->create_fixup(builder->context, inst, labelOperand);
                
            if (fixup->reloc_size == 1) {
                emit_jnz8(builder, inst->operands[0].regnum, &fixup->rom_offset);
            } else if (fixup->reloc_size == 2) {
                emit_jnz16(builder, inst->operands[0].regnum, &fixup->rom_offset);
            } else {
                emit_jnz32(builder, inst->operands[0].regnum, &fixup->rom_offset);
            }
        } break;
        case OPCODE_JCOND: {
            Operand* labelOperand = &inst->operands[2];
            if (!labelOperand->label.len) {
                ERR_IMM_REG();
            }
            LabelFixup* fixup = builder->create_fixup(builder->context, inst, labelOperand);

            if (fixup->reloc_size == 1) {
                emit_jcond8(builder, inst->sub_opcode, inst->operands[0].regnum, inst->operands[1].regnum, &fixup->rom_offset);
            } else if (fixup->reloc_size == 2) {
                emit_jcond16(builder, inst->sub_opcode, inst->operands[0].regnum, inst->operands[1].regnum, &fixup->rom_offset);
            } else {
                emit_jcond32(builder, inst->sub_opcode, inst->operands[0].regnum, inst->operands[1].regnum, &fixup->rom_offset);
            }
        } break;
        case OPCODE_NOT: { EMIT_REG2(not);
        } break;
        case OPCODE_MFCR: {       EMIT_REG2(mfcr);
        } break;
        case OPCODE_MTCR: {       EMIT_REG2(mtcr);
        } break;
        case OPCODE_MSCR: {       EMIT_REG2(mscr);
        } break;
        case OPCODE_CPUFEAT: { EMIT_REG2(cpufeat);
        } break;
        case OPCODE_ADD: { EMIT_REG3(add);
        } break;
        case OPCODE_SUB: { EMIT_REG3(sub);
        } break;
        case OPCODE_UMUL: {  EMIT_REG3(umul);
        } break;
        case OPCODE_SMUL: {      EMIT_REG3(smul);
        } break;
        case OPCODE_UDIV: {     EMIT_REG3(udiv);
        } break;
        case OPCODE_SDIV: {       EMIT_REG3(sdiv);
        } break;
        case OPCODE_UMOD: {       EMIT_REG3(umod);
        } break;
        case OPCODE_SMOD: {       EMIT_REG3(smod);
        } break;
        case OPCODE_AND: {         EMIT_REG3(and);
        } break;
        case OPCODE_OR: {           EMIT_REG3(or);
        } break;
        case OPCODE_XOR: {         EMIT_REG3(xor);
        } break;
        case OPCODE_SHL: {         EMIT_REG3(shl);
        } break;
        case OPCODE_SHR: {         EMIT_REG3(shr);
        } break;
        case OPCODE_RET: {         emit_ret(builder);
        } break;
        case OPCODE_SYSCALL: { emit_syscall(builder);
        } break;
        case OPCODE_VRET: {       emit_vret(builder);
        } break;
        case OPCODE_DBG: {         emit_dbg(builder);
        } break;
        case OPCODE_EINT: {       emit_eint(builder);
        } break;
        case OPCODE_DINT: {       emit_dint(builder);
        } break;
        case OPCODE_SLOW: {       emit_slow(builder);
        } break;
        case OPCODE_WFI: {         emit_wfi(builder);
        } break;
        case OPCODE_NOP: {         emit_nop(builder);
        } break;
        case OPCODE_VMSTART: { emit_vmstart(builder);
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
            Operand* memoryOperand = inst->opcode == OPCODE_CAS ? &inst->operands[2] : &inst->operands[1];
            if (memoryOperand->label.len == 0) {
                emit_memop(builder, inst->sub_opcode, memoryOperand->form, inst->operands[0].regnum, inst->operands[1].regnum, memoryOperand->reg_base, memoryOperand->reg_index, memoryOperand->immediate, NULL);
                break;
            }
            LabelFixup* fixup = builder->create_fixup(builder->context, inst, memoryOperand);

            if (fixup->reloc_size == 1) {
                if (memoryOperand->form == ADDRESSING_PC_DISP32) {
                    // Shrink displacement since label is close.
                    memoryOperand->form = ADDRESSING_PC_DISP8;
                } else if (memoryOperand->form == ADDRESSING_REG1_PC_DISP32) {
                    memoryOperand->form = ADDRESSING_REG1_PC_DISP8;
                }
            } else if (fixup->reloc_size == 2) {
                    if (memoryOperand->form == ADDRESSING_PC_DISP32) {
                    memoryOperand->form = ADDRESSING_PC_DISP16;
                } else if (memoryOperand->form == ADDRESSING_REG1_PC_DISP32) {
                    memoryOperand->form = ADDRESSING_REG1_PC_DISP16;
                }
            }
            emit_memop(builder, inst->sub_opcode, memoryOperand->form, inst->operands[0].regnum, inst->operands[1].regnum, memoryOperand->reg_base, memoryOperand->reg_index, memoryOperand->immediate, &fixup->rom_offset);
        } break;
        case OPCODE_PUSH: {       EMIT_REG1(push);
        } break;
        case OPCODE_POP: {         EMIT_REG1(pop);
        } break;
        case OPCODE_TLBFLUSH: { EMIT_REG1(tlbflush);
        } break;
        case OPCODE_SAVE: {       emit_save(builder, inst->operands[0].regnum, inst->operands[1].immediate);
        } break;
        case OPCODE_RESTORE: {    emit_restore(builder, inst->operands[0].regnum, inst->operands[1].immediate);
        } break;
        case OPCODE_RDTICK:
        case OPCODE_RDTICK1:
        case OPCODE_RDTICK2: {
            if (inst->operands_len == 1) {
                emit_rdtick(builder, inst->operands[0].regnum);
            } else if (inst->operands_len == 2) {
                emit_rdtick1(builder, inst->operands[0].regnum, inst->operands[1].regnum);
            } else if (inst->operands_len == 4) {
                emit_rdtick2(builder, inst->operands[0].regnum, inst->operands[1].regnum, inst->operands[2].regnum, inst->operands[3].regnum);
            } else {
                Assert(false);
            }
        } break;
        case OPCODE_ADVTIMER:
        case OPCODE_ADVTIMER1:
        case OPCODE_ADVTIMER2: {
            if (inst->operands_len == 1) {
                emit_advtimer(builder, inst->operands[0].regnum);
            } else if (inst->operands_len == 2) {
                emit_advtimer1(builder, inst->operands[0].regnum, inst->operands[1].regnum);
            } else if (inst->operands_len == 4) {
                emit_advtimer2(builder, inst->operands[0].regnum, inst->operands[1].regnum, inst->operands[2].regnum, inst->operands[3].regnum);
            } else {
                Assert(false);
            }
        } break;
    } // switch opcode
    
    return true;
}


int regname_to_number(string name) {

    if (name.len > 3) {
             if (equal(name, "CRSTATUS"))    return LWM_REGNR_CRSTATUS;
        else if (equal(name, "CRVB"))        return LWM_REGNR_CRVB;
        else if (equal(name, "CRPT"))        return LWM_REGNR_CRPT;
        else if (equal(name, "CRISP"))       return LWM_REGNR_CRISP;

        else if (equal(name, "CRESTATUS"))   return LWM_REGNR_CRESTATUS;
        else if (equal(name, "CREPC"))       return LWM_REGNR_CREPC;
        else if (equal(name, "CRESP"))       return LWM_REGNR_CRESP;
        else if (equal(name, "CRCAUSE"))     return LWM_REGNR_CRCAUSE;
        else if (equal(name, "CRFAULT"))     return LWM_REGNR_CRFAULT;

        else if (equal(name, "CRCPUID"))     return LWM_REGNR_CRCPUID;
        else if (equal(name, "CRTIMERCMP"))  return LWM_REGNR_CRTIMERCMP;
        else if (equal(name, "CRKERNEL"))    return LWM_REGNR_CRKERNEL;
    }

    if(name.len < 2 || name.len > 3)
        return -1;

    if(name.len == 2) {
             if(name.ptr[0] == 's' && name.ptr[1] == 'p') return LWM_REGNR_SP;
        else if(name.ptr[0] == 'p' && name.ptr[1] == 'c') return LWM_REGNR_PC;
        else if(name.ptr[0] == 'l' && name.ptr[1] == 'r') return LWM_REGNR_LR;
        else if(name.ptr[0] == 't' && name.ptr[1] == 'p') return LWM_REGNR_TP;

        if(name.ptr[0] != 'r')
            return -1;
        if(name.ptr[1] < '0' || name.ptr[1] > '9')
            return -1;
        
        return name.ptr[1] - '0';
    }

    if(name.ptr[0] != 'r')
        return -1;

    if(name.ptr[1] < '0' || name.ptr[1] > '9')
        return -1;
    if(name.ptr[2] < '0' || name.ptr[2] > '9')
        return -1;

    return 10 * (name.ptr[1] - '0') + name.ptr[2] - '0';
}

// #############################3
//     DECODING
// #############################3

#define BITMASK(WORD, BL, BH) ( ((WORD) >> BL) & ( ((uint64_t)1 << ((1+BH)-BL)) - 1 ) )


#define  READ8(address) read_exec_address8(decoder, address)
#define READ16(address) read_exec_address16(decoder, address)
#define READ32(address) read_exec_address32(decoder, address)
#define READ64(address) read_exec_address64(decoder, address)

static inline uint8_t read_exec_address8(Decoder* decoder, uint64_t address) {
    uint8_t value;
    decoder->mmu_operation(decoder->core, MMU_READ_EXEC, address, 1, &value, false);
    return value;
}
static inline uint16_t read_exec_address16(Decoder* decoder, uint64_t address) {
    uint16_t value;
    decoder->mmu_operation(decoder->core, MMU_READ_EXEC, address, 2, &value, false);
    return value;
}
static inline uint32_t read_exec_address32(Decoder* decoder, uint64_t address) {
    uint32_t value;
    decoder->mmu_operation(decoder->core, MMU_READ_EXEC, address, 4, &value, false);
    return value;
}
static inline uint64_t read_exec_address64(Decoder* decoder, uint64_t address) {
    uint64_t value;
    decoder->mmu_operation(decoder->core, MMU_READ_EXEC, address, 8, &value, false);
    return value;
}

int decode_form_reg1_imm(Decoder* decoder, uint64_t pc, uint32_t opcodeBase, uint32_t* opcode, uint8_t regs[1], uint64_t* immediate) {

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


int decode_form_reg1(Decoder* decoder, uint64_t pc, uint32_t* opcode, uint8_t regs[1]) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    regs[0] = READ8(pc+1);

    return 2;
}

int decode_form_reg2(Decoder* decoder, uint64_t pc, uint32_t* opcode, uint8_t regs[2]) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    regs[0] = READ8(pc+1);
    regs[1] = READ8(pc+2);

    return 3;
}

int decode_form_reg3(Decoder* decoder, uint64_t pc, uint32_t* opcode, uint8_t regs[3]) {

    uint32_t tmp_opcode = READ8(pc);
    if (opcode) {
        *opcode = tmp_opcode;
    }

    regs[0] = READ8(pc+1);
    regs[1] = READ8(pc+2);
    regs[2] = READ8(pc+3);

    return 4;
}

int decode_form_reg4(Decoder* decoder, uint64_t pc, uint32_t* opcode, uint8_t regs[4]) {

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

int decode_form_pc(Decoder* decoder, uint64_t pc, uint32_t opcodeBase, uint32_t* opcode, int32_t* immediate) {

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

int decode_form_jmp_reg1(Decoder* decoder, uint64_t pc, uint32_t* opcode, uint8_t regs[1], int32_t* relative) {

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

int decode_form_jmp_reg2(Decoder* decoder, uint64_t pc, uint32_t* opcode, ConditionKind* cond, uint8_t regs[2], int32_t* relative) {

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

int decode_form_memory(Decoder* decoder, uint64_t pc, uint32_t* opcode, uint8_t regs[4], AddressingForm* addressingForm, int64_t* displacement) {
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
            return 0;
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
            return 0;
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
        return 0;
    }

    return 0;
}


int decode_inst(Decoder* decoder, uintptr_t pc, uint32_t* out_opcode, uint8_t* regs, uint64_t* immediate, ConditionKind* cond, AddressingForm* addressingForm) {

    int32_t relative;
    int64_t displacement;
    int bytes = 1;
    uint32_t opcode = READ8(pc);
    *out_opcode = opcode;

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
            bytes = decode_form_reg1_imm(decoder, pc, opcodeBase, NULL, regs, immediate);
        } break;
        case OPCODE_CALL_REG:
        case OPCODE_JMP_REG:
        case OPCODE_PUSH:
        case OPCODE_POP:
        {
            bytes = decode_form_reg1(decoder, pc, NULL, regs);
        } break;
        case OPCODE_NOT:
        case OPCODE_MTCR:
        case OPCODE_MFCR:
        case OPCODE_MSCR:
        case OPCODE_CPUFEAT:
        {
            bytes = decode_form_reg2(decoder, pc, NULL, regs);
            bytes = decode_form_reg2(decoder, pc, NULL, regs);
            bytes = decode_form_reg2(decoder, pc, NULL, regs);
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
            bytes = decode_form_reg3(decoder, pc, NULL, regs);
        } break;
        case OPCODE_JMP:
        case OPCODE_JMP1:
        case OPCODE_JMP2:
        {
            bytes = decode_form_pc(decoder, pc, OPCODE_JMP, NULL, &relative);
            *immediate = relative;
        } break;
        case OPCODE_CALL:
        case OPCODE_CALL1:
        case OPCODE_CALL2:
        {
            bytes = decode_form_pc(decoder, pc, OPCODE_CALL, NULL, &relative);
            *immediate = relative;
        } break;
        case OPCODE_SAVE:
        case OPCODE_RESTORE:
        {
            bytes = decode_form_reg1_imm(decoder, pc, opcode, NULL, regs, immediate);
        } break;
        case OPCODE_RDTICK: {
            bytes = decode_form_reg1(decoder, pc, NULL, regs);
        } break;
        case OPCODE_RDTICK1: {
            bytes = decode_form_reg2(decoder, pc, NULL, regs);
        } break;
        case OPCODE_RDTICK2: {
            bytes = decode_form_reg4(decoder, pc, NULL, regs);
        } break;
        case OPCODE_ADVTIMER:  {
            bytes = decode_form_reg1(decoder, pc, NULL, regs);
        } break;
        case OPCODE_ADVTIMER1: {
            bytes = decode_form_reg2(decoder, pc, NULL, regs);
        } break;
        case OPCODE_ADVTIMER2: {
            bytes = decode_form_reg4(decoder, pc, NULL, regs);
        } break;
        case OPCODE_JZ:
        case OPCODE_JNZ:
        {
            bytes = decode_form_jmp_reg1(decoder, pc, NULL, regs, &relative);
            *immediate = relative;
        } break;
        case OPCODE_JCOND:
        {
            bytes = decode_form_jmp_reg2(decoder, pc, NULL, cond, regs, &relative);
            *immediate = relative;
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
            bytes = decode_form_memory(decoder, pc, NULL, regs, addressingForm, &displacement);
            *immediate = displacement;
        } break;
        case OPCODE_RET:
        case OPCODE_NOP:
        case OPCODE_SYSCALL:
        case OPCODE_VRET:
        case OPCODE_DBG:
        case OPCODE_EINT:
        case OPCODE_DINT:
        case OPCODE_SLOW:
        case OPCODE_WFI:
        case OPCODE_TLBFLUSH:
        case OPCODE_VMSTART:
        {
        } break;
        default: {
            // Invalid instruction
            return 0;
        }
    }
    return bytes;
}