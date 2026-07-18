
#include "lwm/encoding.h"

#include "lwm/builder.h"

bool prepare_encode_inst(string mnemonic, Instruction* inst, int* minBytes, int* maxBytes) {

    #define CASE_OPCODE(STRING, OPCODE) if (equal(mnemonic, STRING)) { inst->opcode = OPCODE; }
    #define CASE_OPCODE2(STRING, OPCODE, SUB_OPCODE) if (equal(mnemonic, STRING)) { inst->opcode = OPCODE; inst->sub_opcode = SUB_OPCODE; }

    if (equal(mnemonic, "mov")) {
        inst->opcode = OPCODE_OR;
        inst->operands[2].regnum = inst->operands[1].regnum;
        inst->operands[1].regnum = inst->operands[1].regnum;
    }
    else if (equal(mnemonic, "hlt")) {
        inst->opcode = OPCODE_STB;
        inst->sub_opcode = MEMOP_STB;
        inst->operands[1].form = ADDRESSING_ABS16;
        inst->operands[1].immediate = 0xF000;
    }
    else if (equal(mnemonic, "li")) {
        inst->opcode = OPCODE_LI64; 
        if (inst->operands[1].immediate >> 63) {
            inst->opcode = OPCODE_LIS8 + (inst->opcode - OPCODE_LI8);
        }
    }
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

