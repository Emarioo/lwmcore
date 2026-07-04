
#include "lwm/lwm_assembler.h"

#include "lwm/asm_types.h"

#include "lwm/util.h"

#include "lwm/lwm_isa.h"

#include "lwm/builder.h"


int parse_space(AssemblerContext* context, int* inout_head);
int parse_line(AssemblerContext* context, int* inout_head);
int parse_name(AssemblerContext* context, int* heainout_headd, string* name);
int parse_string(AssemblerContext* context, int* inout_head, string* str);
int parse_int(AssemblerContext* context, int* inout_head, uint64_t* value);
inline int parse_eof(AssemblerContext* context, int inout_head) {
    return inout_head >= context->text_len;
}

Location count_lines(AssemblerContext* context, int inout_head);
int get_regnr(string name);

#define INVALID_REGISTER -1

char get_char(AssemblerContext* context, int inout_head) {
    if (inout_head >= context->text_len)
        return 0;
    return context->text[inout_head];
}

#define quit() longjmp(context->jumpBuffer, 1)

#define ERROR_SRC_RET(HEAD, FORMAT, ...) Location loc = count_lines(context, HEAD); error_src(loc, FORMAT, ##__VA_ARGS__); quit();
#define ERROR_SRC(HEAD, FORMAT, ...) Location loc = count_lines(context, HEAD); error_src(loc, FORMAT, ##__VA_ARGS__);
#define LOG_SRC(HEAD, FORMAT, ...) Location loc = count_lines(context, HEAD); log_src(loc, FORMAT, ##__VA_ARGS__);



AssemblerError assemble(const char* text, size_t text_len, AssemblerOptions* options) {

    // @TODO Preprocessor


    AssemblerContext _context = {0};
    AssemblerContext* context = &_context;



    int jmpResult = setjmp(context->jumpBuffer);
    if (jmpResult != 0) {
        return LWM_ASM_ERROR_BAD;
    }

    int head = 0;
    uint64_t relative_address = 0;

    while (true) {

        parse_space(context, &head);

        if(parse_eof(context, head)) {
            break;
        }

        int location_head = head;

        string name;
        int parsed_chars = parse_name(context, &head, &name);

        
        int data_bytes = 0;
        if(equal(name, "byte")) {
            data_bytes = 1;
        } else if(equal(name, "short")) {
            data_bytes = 2;
        } else if(equal(name, "long")) {
            data_bytes = 4;
        } else if(equal(name, "quad")) {
            data_bytes = 8;
        }

        if(data_bytes) {
            parse_space(context, &head);

            bool dynamic_length = false;
            uint64_t array_length = 0;
            if(get_char(context, head) == '[') {
                head++;
                
                parse_space(context, &head);

                int loc_int_head = head;
                
                int parsed_chars = parse_int(context, &head, &array_length);
                
                // CHECK_EOF
                char chr = get_char(context, head);
                if(chr != ']') {
                    if(!parsed_chars) {
                        ERROR_SRC_RET(head, "Unexpected character '%c'\n",chr);
                    }
                }
                if(!parsed_chars) {
                    dynamic_length = true;
                }
                if(parsed_chars && array_length == 0) {
                    ERROR_SRC_RET(loc_int_head, "Array length of zero is not allowed.\n");
                }
                if(array_length < 0) {
                    ERROR_SRC_RET(loc_int_head, "Array length cannot be negative! (%d)\n", (int)array_length);
                }
                    
                head++;
                parse_space(context, &head);
            }
            
            // @TODO parse array length

            if (array_length) {

            } else {
                int prev_head = head;
                uint64_t value;
                int parsed_chars = parse_int(context, &head, &value);
                if(parsed_chars) {
                    LOG_SRC(prev_head, "Emit %d\n", (int)value);
                }
                array_length = 1;
            }

            relative_address += data_bytes * array_length;
            continue;
        }

        if (equal(name, "ORG")) {
            // origin, specify address for following labels and instructions
            parse_space(context, &head);
            
            uint64_t addr;
            int parsed_chars = parse_int(context, &head, &addr);
            if(!parsed_chars) {
                ERROR_SRC_RET(head, "Unexpected character '%c'\n", get_char(context, head));
            }
            
            // Determine size for previous block
            context->blocks[context->block_len-1].size = relative_address;

            // Create new block and set its address
            Location loc = count_lines(context, head);
            Block* block = &context->blocks[context->block_len];
            block->addr = addr;
            block->location = loc;
            context->block_len++;
            continue;
        }

        if (!parsed_chars) {
            ERROR_SRC_RET(head, "Unexpected character '%c'\n", get_char(context, head));
        }

        parse_space(context, &head);

        if (get_char(context, head) == ':') {
            // label
            Block* block = &context->blocks[context->block_len-1];
            if (block->label_len >= MAX_LABELS_PER_BLOCK) {
                ERROR_SRC_RET(location_head, "Max label limit reached! (%d)\n", MAX_LABELS_PER_BLOCK);
            }

            // TODO: Alignment

            head++;
            block->labels[block->label_len].addr = relative_address;
            block->labels[block->label_len].name = name;
            block->label_len++;
            continue;
        }

        Instruction inst = {0};

        int operandIndex = 0;

        // Parse operands (if no newline)
        if (get_char(context, head) != '\n')
        while (true) {
            string operand;
            uint64_t value;
            int parsed_chars = parse_name(context, &head, &operand);
            if(parsed_chars) {
                int reg = get_regnr(operand);
                if(reg != INVALID_REGISTER) {
                    inst.operands[operandIndex].regnum = reg;
                    operandIndex++;
                } else {
                    inst.operands[operandIndex].label = operand;
                    operandIndex++;
                }
            } else {
                parsed_chars = parse_int(context, &head, &value);
                if (parsed_chars) {
                    inst.operands[operandIndex].immediate = value;
                    operandIndex++;
                }
            }
            if (!parsed_chars) {
                if (get_char(context ,head) == '[') {
                    // @TODO Memory operand
                } else {
                    ERROR_SRC_RET(location_head, "Could not parse instruction. Skipping to next line. (%s)\n", name.ptr);
                }
            }

            if(get_char(context, head) != ',') {
                break;
            }

            head++;
            parse_space(context, &head);
        }

        inst.operands_len = operandIndex;

        #define CASE_OPCODE(STRING, OPCODE) if (equal(name, STRING)) { inst.opcode = OPCODE; }
        #define CASE_OPCODE2(STRING, OPCODE, SUB_OPCODE) if (equal(name, STRING)) { inst.opcode = OPCODE; inst.sub_opcode = SUB_OPCODE; }


        CASE_OPCODE("li", OPCODE_LI8)
        else CASE_OPCODE("call", OPCODE_CALL)
        else CASE_OPCODE("jmp", OPCODE_JMP)
        else CASE_OPCODE("jz", OPCODE_JZ)
        else CASE_OPCODE("jnz", OPCODE_JNZ)
        else CASE_OPCODE2("jeq", OPCODE_JCOND, COND_EQ)
        else CASE_OPCODE2("jne", OPCODE_JCOND, COND_NE)
        else CASE_OPCODE2("jlt", OPCODE_JCOND, COND_LT)
        else CASE_OPCODE2("jle", OPCODE_JCOND, COND_LE)
        else CASE_OPCODE2("jgt", OPCODE_JCOND, COND_GT)
        else CASE_OPCODE2("jge", OPCODE_JCOND, COND_GE)
        else CASE_OPCODE2("ja",  OPCODE_JCOND, COND_A)
        else CASE_OPCODE2("jae", OPCODE_JCOND, COND_AE)
        else CASE_OPCODE2("jb",  OPCODE_JCOND, COND_B)
        else CASE_OPCODE2("jbe", OPCODE_JCOND, COND_BE)
        else CASE_OPCODE("not", OPCODE_NOT) 
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
        else CASE_OPCODE("lea", OPCODE_LEA) 
        else CASE_OPCODE("ldb", OPCODE_LDB) 
        else CASE_OPCODE("ldbs", OPCODE_LDBS) 
        else CASE_OPCODE("ldh", OPCODE_LDH) 
        else CASE_OPCODE("ldhs", OPCODE_LDHS) 
        else CASE_OPCODE("ldl", OPCODE_LDL) 
        else CASE_OPCODE("ldls", OPCODE_LDLS) 
        else CASE_OPCODE("ldq", OPCODE_LDQ) 
        else CASE_OPCODE("ldq", OPCODE_LDQ) 
        else CASE_OPCODE("stb", OPCODE_STB) 
        else CASE_OPCODE("sth", OPCODE_STH) 
        else CASE_OPCODE("stl", OPCODE_STL) 
        else CASE_OPCODE("stq", OPCODE_STQ) 
        else CASE_OPCODE("push", OPCODE_PUSH) 
        else CASE_OPCODE("pop", OPCODE_POP) 
        else CASE_OPCODE("tlbflush", OPCODE_TLBFLUSH) 
        else CASE_OPCODE("not", OPCODE_NOT) 
        else CASE_OPCODE("mfcr", OPCODE_MFCR) 
        else CASE_OPCODE("mtcr", OPCODE_MTCR) 
        else CASE_OPCODE("cpufeat", OPCODE_CPUFEAT) 
        else CASE_OPCODE("rdtick", OPCODE_RDTICK)
        else {
            ERROR_SRC_RET(location_head, "Unknown instruction '%s'\n", name.ptr);
        }

        Block* block = &context->blocks[context->block_len-1];
        if(block->inst_len >= MAX_INST_PER_BLOCK) {
            ERROR_SRC_RET(location_head, "Max instruction per block reached! (%d)\n", MAX_INST_PER_BLOCK);
        }
        block->insts[block->inst_len++] = inst;
        // relative_address += 2;
    }
    
    // Determine size for previous block
    context->blocks[context->block_len-1].size = relative_address;

    
    //###########################
    //   CONSTRUCT THE BINARY
    //###########################

    // Sort blocks from low to high address.
    int block_len = context->block_len;
    int* sorted_block_indices = malloc(block_len);
    for(int i=0;i<block_len;i++) {
        sorted_block_indices[i] = i;
    }
    for(int j=0;j<block_len;j++) {
        bool changed = false;
        for(int i=0;i<block_len - j - 1;i++) {
            Block* a = &context->blocks[i];
            Block* b = &context->blocks[i+1];
            if(a->addr > b->addr) {
                changed = true;
                int tmp = sorted_block_indices[i+1];
                sorted_block_indices[i+1] = sorted_block_indices[i];
                sorted_block_indices[i] = tmp;
            }
        }
        if(!changed)
            break;
    }

    // Apply absolute address to labels
    for(int i=0;i<block_len;i++) {
        Block* block = &context->blocks[i];
        for(int j=0;j<block->label_len;j++) {
            Label* label = &block->labels[j];
            label->addr += block->addr;
        }
    }

    // Check block overlaps
    // bool has_overlap = false;
    // for(int i=0;i<block_len-1;i++) {
    //     Block* a = &context->blocks[sorted_block_indices[i]];
    //     Block* b = &context->blocks[sorted_block_indices[i+1]];

    //     if(a->addr + a->size > b->addr) {
    //         error("Block overlap!\n");
    //         printf("  %s:%d - [0x%x - 0x%x]\n", a->location.file, a->location.line, a->addr, a->addr + a->size);
    //         printf("  %s:%d - [0x%x - 0x%x]\n", b->location.file, b->location.line, b->addr, b->addr + b->size);
    //         has_overlap = true;
    //     }
    // }
    // if(has_overlap)
    //     return false;

    uint8_t* rom_ptr;
    uint64_t rom_max;
    uint64_t rom_len;

    // Emit blocks to binary
    Block* last_block = &context->blocks[sorted_block_indices[block_len-1]];
    // rom_max = last_block->addr + last_block->size;
    rom_max = last_block->addr + 0x100000;
    rom_ptr = malloc(rom_max);
    rom_len = 0;
    memset(rom_ptr, 0, rom_max);


    Builder  _builder = {0};
    Builder* builder = &_builder;

    // builder_init(builder);

    for(int bi=0;bi<block_len;bi++) {
        Block* block = &context->blocks[sorted_block_indices[bi]];

        builder_init_stream(builder, rom_ptr + block->addr, 0, rom_max - block->addr);

        for(int i=0;i<block->inst_len;i++) {
            Instruction* inst = &block->insts[i];
            
            #define EMIT_REG3(op) \
                emit_##op(builder, inst->operands[0].regnum, inst->operands[1].regnum, inst->operands[2].regnum)
            #define EMIT_REG2(op) \
                emit_##op(builder, inst->operands[0].regnum, inst->operands[1].regnum)
            #define EMIT_REG1(op) \
                emit_##op(builder, inst->operands[0].regnum)
            #define EMIT_MEMOP(kind) \
                emit_memop(builder, kind, inst->operands->form, inst->operands[0].regnum, inst->operands[1].reg_base, inst->operands[1].reg_index, inst->operands[1].immediate)

            switch (inst->opcode) {
                case OPCODE_LI8: {
                    if (inst->operands[1].immediate >> 63) {
                        // Signed
                        int64_t imm = inst->operands[1].immediate;
                        if (imm <= -0x80) {
                            emit_li8(builder, inst->operands[0].regnum, imm);
                        } else if (imm <= -0x8000) {
                            emit_li16(builder, inst->operands[0].regnum, imm);
                        } else if (imm <= -0x80000000) {
                            emit_li32(builder, inst->operands[0].regnum, imm);
                        } else {
                            emit_li64(builder, inst->operands[0].regnum, imm);
                        }
                    } else {
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
                    }
                } break;
                case OPCODE_CALL: {

                } break;
                case OPCODE_JMP: {
                } break;
                case OPCODE_JZ: {
                } break;
                case OPCODE_JNZ: {
                // @TODO jlt, jge
                } break;
                case OPCODE_NOT: {
                    EMIT_REG2(not);
                } break;
                case OPCODE_ADD: {
                    EMIT_REG3(add);
                } break;
                case ("sub", OPCODE_SUB) {         EMIT_REG3(sub);
                } break;
                case ("umul", OPCODE_UMUL) {       EMIT_REG3(umul);
                } break;
                case ("smul", OPCODE_SMUL) {       EMIT_REG3(smul);
                } break;
                case ("udiv", OPCODE_UDIV) {       EMIT_REG3(udiv);
                } break;
                case ("sdiv", OPCODE_SDIV) {       EMIT_REG3(sdiv);
                } break;
                case ("umod", OPCODE_UMOD) {       EMIT_REG3(umod);
                } break;
                case ("smod", OPCODE_SMOD) {       EMIT_REG3(smod);
                } break;
                case ("and", OPCODE_AND) {         EMIT_REG3(and);
                } break;
                case ("or", OPCODE_OR) {           EMIT_REG3(or);
                } break;
                case ("xor", OPCODE_XOR) {         EMIT_REG3(xor);
                } break;
                case ("shl", OPCODE_SHL) {         EMIT_REG3(shl);
                } break;
                case ("ret", OPCODE_RET) {         emit_ret(builder);
                } break;
                case ("syscall", OPCODE_SYSCALL) { emit_syscall(builder);
                } break;
                case ("vret", OPCODE_VRET) {       emit_vret(builder);
                } break;
                case ("dbg", OPCODE_DBG) {         emit_dbg(builder);
                } break;
                case ("eint", OPCODE_EINT) {       emit_eint(builder);
                } break;
                case ("dint", OPCODE_DINT) {       emit_dint(builder);
                } break;
                case ("slow", OPCODE_SLOW) {       emit_slow(builder);
                } break;
                case ("wfi", OPCODE_WFI) {         emit_wfi(builder);
                } break;
                case ("nop", OPCODE_NOP) {         emit_nop(builder);
                } break;
                case ("vmstart", OPCODE_VMSTART) { emit_vmstart(builder);
                } break;
                case ("lea", OPCODE_LEA) {         EMIT_MEMOP(MEMOP_LEA);
                } break;
                case ("ldb", OPCODE_LDB) {         EMIT_MEMOP(MEMOP_LDB);
                } break;
                case ("ldbs", OPCODE_LDBS) {       EMIT_MEMOP(MEMOP_LDBS);
                } break;
                case ("ldh", OPCODE_LDH) {         EMIT_MEMOP(MEMOP_LDH);
                } break;
                case ("ldhs", OPCODE_LDHS) {       EMIT_MEMOP(MEMOP_LDHS);
                } break;
                case ("ldl", OPCODE_LDL) {         EMIT_MEMOP(MEMOP_LDL);
                } break;
                case ("ldls", OPCODE_LDLS) {       EMIT_MEMOP(MEMOP_LDLS);
                } break;
                case ("ldq", OPCODE_LDQ) {         EMIT_MEMOP(MEMOP_LDQ);
                } break;
                case ("ldq", OPCODE_LDQ) {         EMIT_MEMOP(MEMOP_LDQ);
                } break;
                case ("stb", OPCODE_STB) {         EMIT_MEMOP(MEMOP_STB);
                } break;
                case ("sth", OPCODE_STH) {         EMIT_MEMOP(MEMOP_STH);
                } break;
                case ("stl", OPCODE_STL) {         EMIT_MEMOP(MEMOP_STL);
                } break;
                case ("stq", OPCODE_STQ) {         EMIT_MEMOP(MEMOP_STQ);
                } break;
                case ("push", OPCODE_PUSH) {       EMIT_REG1(push);
                } break;
                case ("pop", OPCODE_POP) {         EMIT_REG1(pop);
                } break;
                case ("tlbflush", OPCODE_TLBFLUSH) { EMIT_REG1(tlbflush);
                } break;
                case ("not", OPCODE_NOT) {         EMIT_REG2(not);
                } break;
                case ("mfcr", OPCODE_MFCR) {       EMIT_REG2(mfcr);
                } break;
                case ("mtcr", OPCODE_MTCR) {       EMIT_REG2(mtcr);
                } break;
                case ("cpufeat", OPCODE_CPUFEAT) { EMIT_REG2(cpufeat);
                } break;
                case ("rdtick", OPCODE_RDTICK) {
                
                if (inst->operands_len == 1) {
                    emit_rdtick(builder, inst->operands[0].regnum);
                } else if (inst->operands_len == 2) {
                    emit_rdtick1(builder, inst->operands[0].regnum, inst->operands[1].regnum);
                } else if (inst->operands_len == 4) {
                    emit_rdtick2(builder, inst->operands[0].regnum, inst->operands[1].regnum, inst->operands[2].regnum, inst->operands[3].regnum);
                } else {
                    Assert(false);
                }


            }



            u16 word = 0;

            // Emit instructions
            if(inst->opcode == INST_LI) {
                Assert(inst->regs[0] >= 0 && inst->regs[0] <= 0xF);
                // word = (inst->opcode) | (inst->regs[0] << 4) | ((inst->imm << 8) & 0xFF00);
                word = (inst->opcode << 12) | (inst->regs[0] << 8) | ((inst->imm) & 0xFF);
            } else if(inst->opcode == INST_RET) {
                word = (inst->opcode << 8);
            } else if(inst->opcode == INST_JMP || inst->opcode == INST_CALL) {
                word = (inst->opcode << 12) | ((inst->imm) & 0xFFF);
            } else if(inst->opcode >= INST_SHL && inst->opcode <= INST_SHR) {
                Assert((inst->regs[0] &~0xF) == 0);
                Assert(inst->imm >= 0 && inst->imm <= 15);
                word = (inst->opcode << 8) | (inst->regs[0] << 4) | (inst->imm);
            } else if(inst->opcode >= INST_LODW && inst->opcode <= INST_SHR) {
                Assert((inst->regs[0] &~0xF) == 0);
                Assert((inst->regs[1] &~0xF) == 0);
                word = (inst->opcode << 8) | (inst->regs[0] << 4) | (inst->regs[1]);
            }


            *(i16*)&rom_ptr[rom_len] = word;
            rom_len += 2;
        }
    }

    return LWM_ASM_ERROR_NONE;

}



int parse_name(AssemblerContext* context, int* inout_head, string* name) {
    name->len = 0;
    name->ptr = NULL;
    int origin_head = *inout_head;
    int head = *inout_head;

    char* text = context->text;
    int text_len = context->text_len;

    char chr = text[head];
    if (!(((chr|32) >= 'a' && (chr|32) <= 'z') || chr == '_')) {
        return 0;
    }
    char* start_ptr = &text[head];
    while(head < text_len) {
        chr = text[head];
        if (((chr|32) >= 'a' && (chr|32) <= 'z') || (chr >= '0' && chr <= '9') || chr == '_') {
            head++;
            name->len++;
            continue;
        }
        break;
    }
    // TODO: Memory leak, we allocate new string to null terminate it. Other characters how up otherwise.
    char* buf = (char*)malloc(name->len + 1);
    memcpy(buf, start_ptr, name->len);
    buf[name->len] = '\0';
    name->ptr = buf;
    // name->max = name->len;

    *inout_head = head;
    return head - origin_head;
}

int parse_int(AssemblerContext* context, int* inout_head, uint64_t* value) {
    int origin_head = *inout_head;
    int head = *inout_head;
    
    char* text = context->text;
    int text_len = context->text_len;

    uint64_t tempValue = 0;
    *value = 0;

    bool negate = false;
    if(text[head] == '-') {
        negate = true;
        ++head;
    }

    if (head + 2 < text_len && text[head] == '0' && text[head+1] == 'x') {
        // 16-base hexidecimal
        head += 2;
        while(head < text_len) {
            char chr = text[head];
            if(chr == '_') {
                ++head;
                continue;
            }
            if (chr >= '0' && chr <= '9') {
                ++head;
                tempValue = 16 * tempValue + (chr|32) - '0';
                continue;
            }
            if ((chr|32) >= 'a' && (chr|32) <= 'f') {
                ++head;
                tempValue = 16 * tempValue + (chr|32) - 'a';
                continue;
            }
            break;
        }
     } else if (head + 2 < text_len && text[head] == '0' && text[head+1] == 'b') {
        // 2-base hexidecimal
        head += 2;
        while(head < text_len) {
            char chr = text[head];
            if(chr == '_') {
                ++head;
                continue;
            }
            if (chr >= '0' && chr <= '1') {
                ++head;
                tempValue = 2 * tempValue + chr - '0';
                continue;
            }
            break;
        }
    } else if (head + 2 < text_len && text[head] == '0' && text[head+1] == 'o') {
        // 8-base hexidecimal
        head += 2;
        while(head < text_len) {
            char chr = text[head];
            if(chr == '_') {
                ++head;
                continue;
            }
            if (chr >= '0' && chr <= '7') {
                ++head;
                tempValue = 8 * tempValue + chr - '0';
                continue;
            }
            break;
        }
    } else {
        // 10-base integer
        while(head < text_len) {
            char chr = text[head];
            if(chr == '_') {
                ++head;
                continue;
            }
            if (chr >= '0' && chr <= '9') {
                ++head;
                tempValue = 10 * tempValue + chr - '0';
                continue;
            }
            break;
        }
    }
    if(negate) {
        *value = -tempValue;
    } else {
        *value = tempValue;
    }
    *inout_head = head;
    return head - origin_head;
}

int parse_space(AssemblerContext* context, int* inout_head) {

    int origin_head = *inout_head;
    int head = *inout_head;

    char* text = context->text;
    int text_len = context->text_len;


    while(head < text_len) {
        char chr = text[head];
        char chr_next = 0;
        if (head + 1 < text_len) {
            chr_next = text[head + 1];
        }

        if ((chr == '#' && (chr == 0 || chr == ' ' || chr == '\n' )) || chr == ';' || (chr == '/' && chr_next == '/')) {
            head++;
            if(chr == '/') {
                head++;
            }
            while(head < text_len) {
                if(text[head] == '\n') {
                    break;
                }
                head++;
            }
            continue;
        }
        if (chr == '/' && chr_next == '*') {
            head += 2;
            while(head < text_len) {
                if(head+2 < text_len && text[head] == '*' && text[head+1] == '/') {
                    head+=2;
                    break;
                }
                ++head;
            }
            continue;
        }

        if(chr == ' ' || chr == '\r' || chr == '\f' || chr == '\t' || chr == '\n') {
            ++head;
            continue;
        }
        break;
    }
    *inout_head = head;
    return head - origin_head;
}

int parse_string(AssemblerContext* context, int* inout_head, string* str) {
    char* text = context->text;
    int text_len = context->text_len;

    if(str) {
        str->len = 0;
        str->ptr = NULL;
    }
    int origin_head = *inout_head;
    int head = *inout_head;

    char chr = text[head];
    if(chr != '"') {
        return 0;
    }
    ++head;
    if(str) {
        str->ptr = &text[head];
    }
    while(head < text_len) {
        // TODO: Handle escaped characters
        if(text[head] == '"') {
            ++head;
            break;
        }
        ++head;
        if(str) {
            str->len++;
        }
    }

    if(str) {
        // IMPORTANT: Do not remove this allocation, code when processing includes relies
        //   on the returned memory not being invalidated.
        // IMPORTANT: Code also relies on it being null terminated at its proper length.
        char* buf = malloc(str->len + 1);
        memcpy(buf, str->ptr, str->len);
        buf[str->len] = '\0';
        str->ptr = buf;
    }
    *inout_head = head;
    return head - origin_head;
}

int parse_line(AssemblerContext* context, int* inout_head) {
    int origin_head = *inout_head;
    int head = *inout_head;

    char* text = context->text;
    int text_len = context->text_len;

    while(head < text_len) {
        if(text[head] == '\n')
            break;
        ++head;
    }
    *inout_head = head;
    return head - origin_head;
}



Location count_lines(AssemblerContext* context, int inout_head) {
    // Assert(head < text->len);
    int head = inout_head;
    char* text = context->text;

    LocationMapping* map = nullptr;
    for(int i=0;i<context->loc_map_len;i++) {
        map = &context->loc_map[i];
        if(head >= map->head_start && head < map->head_end)
            break;
    }
    // Assert(map);
    Location loc = {map->file.ptr, map->line, 1};
    int index = map->head_start;
    while(index < head) {
        if (text[index] == '\n') {
            loc.line++;
            loc.column = 0;
        }

        index++;
        loc.column++;
    }
    return loc;
}



int get_regnr(string name) {

    if (name.len > 3) {
             if (equal(name, "CRSTATUS"))    return LWM_REGNR_CRSTATUS;
        else if (equal(name, "CRVB"))        return LWM_REGNR_CRVB;
        else if (equal(name, "CRPT"))        return LWM_REGNR_CRPT;
        else if (equal(name, "CREPC"))       return LWM_REGNR_CREPC;
        else if (equal(name, "CRCAUSE"))     return LWM_REGNR_CRCAUSE;
        else if (equal(name, "CRFAULT"))     return LWM_REGNR_CRFAULT;
        else if (equal(name, "CRCPUID"))     return LWM_REGNR_CRCPUID;
        else if (equal(name, "CRTIMERCMP"))  return LWM_REGNR_CRTIMERCMP;
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
