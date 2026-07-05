
#include "lwm/assembler.h"

#include "lwm/asm_types.h"

#include "lwm/util.h"
#include "lwm/isa.h"

#include "lwm/builder.h"
#include "lwm/parser_util.h"


typedef struct {
    string source_file;
    int error_code;

    char* text;
    int   text_len;

    
    int      sections_len;
    Section* sections;

    ParserContext parserContext;

    jmp_buf jumpBuffer;
} AssemblerContext;





#define parse_space(CTX, ...) parse_space(&CTX->parserContext, __VA_ARGS__)
#define parse_line(CTX, ...) parse_line(&CTX->parserContext, __VA_ARGS__)
#define parse_name(CTX, ...) parse_name(&CTX->parserContext, __VA_ARGS__)
#define parse_non_space(CTX, ...) parse_non_space(&CTX->parserContext, __VA_ARGS__)
#define parse_space_not_newline(CTX, ...) parse_space_not_newline(&CTX->parserContext, __VA_ARGS__)
#define parse_string(CTX, ...) parse_string(&CTX->parserContext, __VA_ARGS__)
#define parse_int(CTX, ...) parse_int(&CTX->parserContext, __VA_ARGS__)
#define parse_char(CTX, ...) parse_char(&CTX->parserContext, __VA_ARGS__)
#define parse_eof(CTX, ...) parse_eof(&CTX->parserContext, __VA_ARGS__)
#define count_lines(CTX, ...) count_lines(&CTX->parserContext, __VA_ARGS__)
#define get_char(CTX, ...) get_char(&CTX->parserContext, __VA_ARGS__)

int get_regnr(string name);

#define INVALID_REGISTER -1

#define ERROR_SRC_RET(HEAD, FORMAT, ...) Location loc = count_lines(context, HEAD); error_src(loc, FORMAT, ##__VA_ARGS__); longjmp(context->jumpBuffer, 1)
#define ERROR_SRC(HEAD, FORMAT, ...) Location loc = count_lines(context, HEAD); error_src(loc, FORMAT, ##__VA_ARGS__);
#define LOG_SRC(HEAD, FORMAT, ...) Location loc = count_lines(context, HEAD); log_src(loc, FORMAT, ##__VA_ARGS__);


bool find_label(AssemblerContext* context, const char* name, Section** out_section, Label** out_label) {
    for (int si=0;si<context->sections_len;si++) {
        Section* section = &context->sections[si];
        for (int li=0;li<section->label_len;li++) {
            Label* label = &section->labels[li];
            if (equal(label->name, name)) {
                *out_section = section;
                *out_label = label;
                return true;
            }
        }
    }
    return false;
}


AssemblerError assemble(const char* in_text, size_t in_text_len, AssemblerOptions* options) {

    string out_text;
    bool preprocResult = preprocess_text((string){ .len = in_text_len, .ptr = (char*)in_text }, &out_text);
    if (!preprocResult) {
        return LWM_ASM_ERROR_BAD;
    }

    writeFile("temp.pp", out_text.ptr, out_text.len);

    const char* text = out_text.ptr;
    int text_len = out_text.len;

    AssemblerContext* context = malloc(sizeof(AssemblerContext));
    memset(context, 0, sizeof(AssemblerContext));
    // Memory leak. we never free context.

    context->source_file.ptr = (char*)options->sourcePath;
    context->parserContext.path = (char*)context->source_file.ptr;
    context->parserContext.text = (char*)text;
    context->parserContext.text_len = text_len;

    context->sections_len = 0;
    int max_sections = 20;
    context->sections = malloc(sizeof(Section) * max_sections);
    memset(context->sections, 0, sizeof(Section) * max_sections);
    
    // LocationMapping* map = &context->loc_map[context->loc_map_len++];
    // map->file = context->source_file;
    // map->head_start = 0;
    // map->line = 1;
    // map->head_end = text_len;

    int jmpResult = setjmp(context->jumpBuffer);
    if (jmpResult != 0) {
        return LWM_ASM_ERROR_BAD;
    }

    {
        Location loc = count_lines(context, 0);
        Section* section = &context->sections[context->sections_len];
        section->addr = 0;
        section->location = loc;
        context->sections_len++;

        section->label_len = 0;
        section->objects_len = 0;
        section->labels = malloc(sizeof(Label) * 100);
        section->objects = malloc(sizeof(Object) * 100);
        memset(section->labels, 0, sizeof(Label) * 100);
        memset(section->objects, 0, sizeof(Object) * 100);
    }

    int head = 0;
    uint64_t estimated_address = 0; // relative to block

    uint8_t queuedAlignment = 0;

    while (true) {

        parse_space(context, &head);

        if(parse_eof(context, head)) {
            break;
        }

        int location_head = head;

        string name;
        int parsed_chars = parse_name(context, &head, &name);
        if (!parsed_chars) {
            char chr = get_char(context, head);
            ERROR_SRC_RET(head, "Unexpected character '%c'\n", chr);
        }
        
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

                u8* bytes = malloc(data_bytes);
                memcpy(bytes, &value, data_bytes);

                // @TODO Check object limit

                Section* currentSection = &context->sections[context->sections_len-1];
                Object* object = &currentSection->objects[currentSection->objects_len];
                currentSection->objects_len++;

                object->kind = OBJECT_DATA;
                object->alignment = queuedAlignment;
                queuedAlignment = 0;
                object->dataobj.bytes = bytes;
                object->dataobj.size = data_bytes;
            }

            estimated_address += data_bytes * array_length;
            continue;
        }

        if (equal(name, "section")) {
            // origin, specify address for following labels and instructions
            parse_space(context, &head);

            string sectionName;
            parse_non_space(context, &head, &sectionName);

            parse_space(context, &head);

            context->sections[context->sections_len-1].size = estimated_address;

            // Create new block and set its address
            Location loc = count_lines(context, head);
            Section* section = &context->sections[context->sections_len];
            section->location = loc;
            context->sections_len++;
            
            uint64_t addr;
            int parsed_chars = parse_int(context, &head, &addr);
            if(parsed_chars) {
                section->addr = addr;
            }

            section->label_len = 0;
            section->objects_len = 0;
            section->labels = malloc(sizeof(Label) * 100);
            section->objects = malloc(sizeof(Object) * 100);
            memset(section->labels, 0, sizeof(Label) * 100);
            memset(section->objects, 0, sizeof(Object) * 100);

            estimated_address = 0;

            continue;
        }

        if (equal(name, "align")) {
            // origin, specify address for following labels and instructions
            parse_space(context, &head);

            uint64_t alignment;
            int parsed_chars = parse_int(context, &head, &alignment);
            if(!parsed_chars) {
                ERROR_SRC_RET(head, "Expected an integer, not '%c'\n", get_char(context, head));
            }

            queuedAlignment = 32 - lzcnt(alignment);

            continue;
        }

        if (!parsed_chars) {
            ERROR_SRC_RET(head, "Unexpected character '%c'\n", get_char(context, head));
        }

        parse_space_not_newline(context, &head);

        if (get_char(context, head) == ':') {
            // label
            Section* currentSection = &context->sections[context->sections_len-1];
            // if (currentSection->label_len >= MAX_LABELS_PER_BLOCK) {
            //     ERROR_SRC_RET(location_head, "Max label limit reached! (%d)\n", MAX_LABELS_PER_BLOCK);
            // }

            head++;
            Label* label = &currentSection->labels[currentSection->label_len];
            currentSection->label_len++;

            label->estimated_address = estimated_address;
            label->name = name;
            label->object_id = currentSection->objects_len; // Refer to next object
            continue;
        }

        Instruction inst = {0};
        inst.parseHead = location_head;

        int operandIndex = 0;

        // Parse operands (if no newline and not EOF)
        if ((get_char(context, head) != '\n' && get_char(context, head) != '\r') && !parse_eof(context, head)) {
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
                } else {
                    char chrValue;
                    parsed_chars = parse_char(context, &head, &chrValue);
                    if (parsed_chars) {
                        inst.operands[operandIndex].immediate = chrValue;
                        operandIndex++;
                    }
                }
            }
            if (!parsed_chars) {
                if (get_char(context, head) == '[') {
                    // Parse memory addressing
                    //   [base + index + displacement]
                    // displacement can be made up of one label and multiple integers that are added
                    head++;
                    
                    Operand* memoryOperand = &inst.operands[operandIndex];
                    operandIndex++;

                    int parsedRegs = 0;
                    int hasLabel = false;

                    while (true) {

                        parse_space(context, &head);

                        if (parse_eof(context, head)) {
                            ERROR_SRC_RET(head, "Expected ] to end memory operand, got end of file.\n");
                        }

                        char chr = get_char(context, head);
                        if (chr == ']') {
                            head++;
                            break;
                        }

                        string word;
                        uint64_t disp;
                        int parsed_chars = parse_name(context, &head, &word);
                        if(parsed_chars) {
                            int reg = get_regnr(word);
                            if(reg != INVALID_REGISTER) {
                                if (parsedRegs == 0) {
                                    memoryOperand->reg_base = reg;
                                } else if (parsedRegs == 1) {
                                    memoryOperand->reg_index = reg;
                                } else {
                                    ERROR_SRC_RET(head, "Memory addressing is limited to 2 registers.\n");
                                }
                                parsedRegs++;
                            } else {
                                if (hasLabel) {
                                    ERROR_SRC_RET(head, "Memory addressing is limited to one label.\n");
                                } else {
                                    memoryOperand->label = operand;
                                }
                            }
                        } else {
                            parsed_chars = parse_int(context, &head, &value);
                            if (parsed_chars) {
                                memoryOperand->immediate += value;
                            } else {
                                char chrValue;
                                parsed_chars = parse_char(context, &head, &chrValue);
                                if (parsed_chars) {
                                    memoryOperand->immediate += chrValue;
                                }
                            }
                        }
                    }

                } else {
                    ERROR_SRC_RET(location_head, "Could not parse instruction. Skipping to next line. (%s)\n", name.ptr);
                }
            }
            
            parse_space(context, &head);

            if(get_char(context, head) != ',') {
                break;
            }
            head++;

            parse_space(context, &head);
        }
        } // if not newline && not EOF

        inst.operands_len = operandIndex;

        #define CASE_OPCODE(STRING, OPCODE) if (equal(name, STRING)) { inst.opcode = OPCODE; }
        #define CASE_OPCODE2(STRING, OPCODE, SUB_OPCODE) if (equal(name, STRING)) { inst.opcode = OPCODE; inst.sub_opcode = SUB_OPCODE; }

        if (equal(name, "mov")) {
            inst.opcode = OPCODE_OR;
            inst.operands[2].regnum = inst.operands[1].regnum;
            inst.operands[1].regnum = inst.operands[0].regnum;
        }
        else CASE_OPCODE("li", OPCODE_LI8)
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
        else CASE_OPCODE("mfcr", OPCODE_MFCR) 
        else CASE_OPCODE("mtcr", OPCODE_MTCR) 
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
        else CASE_OPCODE("stb", OPCODE_STB) 
        else CASE_OPCODE("sth", OPCODE_STH) 
        else CASE_OPCODE("stl", OPCODE_STL) 
        else CASE_OPCODE("stq", OPCODE_STQ) 
        else CASE_OPCODE("push", OPCODE_PUSH) 
        else CASE_OPCODE("pop", OPCODE_POP) 
        else CASE_OPCODE("tlbflush", OPCODE_TLBFLUSH) 
        else CASE_OPCODE("rdtick", OPCODE_RDTICK)
        else {
            ERROR_SRC_RET(location_head, "Unknown instruction '%s'\n", name.ptr);
        }
        
        Section* currentSection = &context->sections[context->sections_len-1];
        Object* object = &currentSection->objects[currentSection->objects_len];
        currentSection->objects_len++;

        object->kind = OBJECT_INSTRUCTION;
        object->alignment = queuedAlignment;
        queuedAlignment = 0;
        object->inst = inst;
        // @TODO ERROR_SRC_RET(location_head, "Max instruction per block reached! (%d)\n", MAX_INST_PER_BLOCK);
        
        int instBytes = largest_encoding(inst.opcode, inst.operands[1].form);

        estimated_address += instBytes;
    }
    
    context->sections[context->sections_len-1].size = estimated_address;
    
    
    //###########################
    //   CONSTRUCT THE BINARY
    //###########################

    // Sort blocks from low to high address.
    int section_len = context->sections_len;
    int* sorted_section_indices = malloc(section_len);
    for(int i=0;i<section_len;i++) {
        sorted_section_indices[i] = i;
    }
    for(int j=0;j<section_len;j++) {
        bool changed = false;
        for(int i=0;i<section_len - j - 1;i++) {
            Section* a = &context->sections[i];
            Section* b = &context->sections[i+1];
            if(a->addr > b->addr) {
                changed = true;
                int tmp = sorted_section_indices[i+1];
                sorted_section_indices[i+1] = sorted_section_indices[i];
                sorted_section_indices[i] = tmp;
            }
        }
        if(!changed)
            break;
    }

    // Apply absolute address to labels
    for(int i=0;i<section_len;i++) {
        Section* section = &context->sections[i];
        for(int j=0;j<section->label_len;j++) {
            Label* label = &section->labels[j];
            label->estimated_address += section->addr;
            // label->final_address += section->addr;
        }
    }

    // Check section overlaps
    // bool has_overlap = false;
    // for(int i=0;i<section_len-1;i++) {
    //     Section* a = &context->sections[sorted_section_indices[i]];
    //     Section* b = &context->sections[sorted_section_indices[i+1]];

    //     if(a->addr + a->size > b->addr) {
    //         error("Section overlap!\n");
    //         printf("  %s:%d - [0x%zx - 0x%zx]\n", a->location.file, a->location.line, a->addr, a->addr + a->size);
    //         printf("  %s:%d - [0x%zx - 0x%zx]\n", b->location.file, b->location.line, b->addr, b->addr + b->size);
    //         has_overlap = true;
    //     }
    // }
    // if(has_overlap)
    //     return false;

    uint8_t* rom_ptr;
    uint64_t rom_max;
    uint64_t rom_len;

    // Emit sections to binary
    Section* last_section = &context->sections[sorted_section_indices[section_len-1]];
    rom_max = last_section->addr + last_section->size; // sections should have worst case size
    rom_ptr = malloc(rom_max);
    rom_len = 0;
    memset(rom_ptr, 0, rom_max);

    typedef struct {
        Label*   label;
        uint64_t rom_offset;
        uint8_t  reloc_size;
    } LabelFixup;

    int         labelFixups_len = 0;
    LabelFixup* labelFixups = malloc(sizeof(LabelFixup) * 1000);

    Builder  _builder = {0};
    Builder* builder = &_builder;

    for(int bi=0;bi<section_len;bi++) {
        Section* section = &context->sections[sorted_section_indices[bi]];

        builder_init_stream(builder, rom_ptr, section->addr, rom_max);

        int checkLabelIndex = 0;

        for(int oi=0;oi<section->objects_len;oi++) {
            Object* object = &section->objects[oi];

            // Apply alignment
            int wantedAlignment = 1 << object->alignment;
            int alignment = builder_head(builder) % wantedAlignment;
            if (alignment != 0) {
                emit_zeros(builder, wantedAlignment - alignment);
            }

            /* @TODO We have a bug in this case where label will be aligned
                 when it shouldn't.
            myvar:
                align 16
                long 0x202

                We have another bug here since alignment is attached on objects
                and there is no object after last label.
            myvar_start:
                long 0x202
                align 16
            myvar_end:
            */

            // Update label addresses
            while (checkLabelIndex < section->label_len) {
                Label* label = &section->labels[checkLabelIndex];
                if (label->object_id != oi) {
                    break;
                }
                label->final_address = builder_head(builder);
                checkLabelIndex++;
                // printf("Update label %s 0x%x obj=%d\n", label->name.ptr, label->final_address, label->object_id);
            }

            if (object->kind == OBJECT_DATA) {
                emit_bytes(builder, object->dataobj.bytes, object->dataobj.size);
                continue;
            }

            Instruction* inst = &object->inst;
            
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
                    Operand* labelOperand = &inst->operands[0];
                    if (labelOperand->label.len) {
                        LabelFixup* fixup = &labelFixups[labelFixups_len];
                        labelFixups_len++;

                        Section* foundSection;
                        Label* foundLabel;
                        bool found = find_label(context, labelOperand->label.ptr, &foundSection, &foundLabel);
                        if (found) {
                            uint64_t pc = builder_head(builder);
                            int64_t relative = foundLabel->estimated_address - pc;

                            if (abs(relative) < 0x80) {
                                emit_call8(builder, &fixup->rom_offset);
                                fixup->reloc_size = 1;
                            } else if (abs(relative) < 0x8000) {
                                emit_call16(builder, &fixup->rom_offset);
                                fixup->reloc_size = 2;
                            } else {
                                emit_call32(builder, &fixup->rom_offset);
                                fixup->reloc_size = 4;
                            }
                            fixup->label = foundLabel;
                        } else {
                            ERROR_SRC_RET(inst->parseHead, "Label %s does not exist.\n", labelOperand->label.ptr);
                        }
                    } else {
                        // Immediate
                        Assert(false);
                    }
                } break;
                case OPCODE_JMP: {
                } break;
                case OPCODE_JZ: {
                } break;
                case OPCODE_JNZ: {
                // @TODO jlt, jge
                } break;
                case OPCODE_NOT: { EMIT_REG2(not);
                } break;
                case OPCODE_MFCR: {       EMIT_REG2(mfcr);
                } break;
                case OPCODE_MTCR: {       EMIT_REG2(mtcr);
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
                case OPCODE_LEA: {         EMIT_MEMOP(MEMOP_LEA);
                } break;
                case OPCODE_LDB: {         EMIT_MEMOP(MEMOP_LDB);
                } break;
                case OPCODE_LDBS: {       EMIT_MEMOP(MEMOP_LDBS);
                } break;
                case OPCODE_LDH: {         EMIT_MEMOP(MEMOP_LDH);
                } break;
                case OPCODE_LDHS: {       EMIT_MEMOP(MEMOP_LDHS);
                } break;
                case OPCODE_LDL: {         EMIT_MEMOP(MEMOP_LDL);
                } break;
                case OPCODE_LDLS: {       EMIT_MEMOP(MEMOP_LDLS);
                } break;
                case OPCODE_LDQ: {         EMIT_MEMOP(MEMOP_LDQ);
                } break;
                case OPCODE_STB: {         EMIT_MEMOP(MEMOP_STB);
                } break;
                case OPCODE_STH: {         EMIT_MEMOP(MEMOP_STH);
                } break;
                case OPCODE_STL: {         EMIT_MEMOP(MEMOP_STL);
                } break;
                case OPCODE_STQ: {         EMIT_MEMOP(MEMOP_STQ);
                } break;
                case OPCODE_PUSH: {       EMIT_REG1(push);
                } break;
                case OPCODE_POP: {         EMIT_REG1(pop);
                } break;
                case OPCODE_TLBFLUSH: { EMIT_REG1(tlbflush);
                } break;
                case OPCODE_RDTICK: {
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
            }
        }
        size_t rom_stream_offset = builder->byteStream - rom_ptr;
        size_t rom_stream_length = builder->byteStream_len;
        if (rom_stream_offset + rom_stream_length > rom_len)
            rom_len = rom_stream_offset + rom_stream_length;
    }

    for (int li=0;li<labelFixups_len;li++) {
        LabelFixup* fixup = &labelFixups[li];

        int64_t relative = fixup->label->final_address - (fixup->rom_offset + fixup->reloc_size);
        if (fixup->reloc_size == 1) {
            if (abs(relative) >= 0x80) {
                printf("Label fixup to big for rel%d (value %zd)\n", fixup->reloc_size*8, relative);
            }
            // printf("Fixup *0x%zx = %zd\n", fixup->rom_offset, relative);
            *(int8_t*)(rom_ptr + fixup->rom_offset) = relative;
        } else if (fixup->reloc_size == 2) {
            if (abs(relative) >= 0x8000) {
                printf("Label fixup to big for rel%d (value %zd)\n", fixup->reloc_size*8, relative);
            }
            *(int16_t*)(rom_ptr + fixup->rom_offset) = relative;
        } else {
            *(int32_t*)(rom_ptr + fixup->rom_offset) = relative;
        }

    }

    options->rom = rom_ptr;
    options->rom_len = rom_len;
    options->rom_max = rom_max;

    return LWM_ASM_ERROR_NONE;

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
