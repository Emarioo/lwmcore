
#include "lwm/assembler.h"

#include "lwm/asm_types.h"

#include "lwm/util.h"
#include "lwm/isa.h"

#include "lwm/builder.h"
#include "lwm/parser_util.h"

#include "lwm/encoding.h"



    
typedef struct {
    string source_file;
    int error_code;

    char* text;
    int   text_len;

    int         labelFixups_max;
    int         labelFixups_len;
    LabelFixup* labelFixups;
    
    int      sections_max;
    int      sections_len;
    Section* sections;

    Builder* builder;

    ParserContext parserContext;

    jmp_buf jumpBuffer;
} AssemblerContext;


#define MAX_OBJECTS 400
#define MAX_LABELS 100


#define parse_space(CTX, ...) parse_space(&CTX->parserContext, __VA_ARGS__)
#define parse_line(CTX, ...) parse_line(&CTX->parserContext, __VA_ARGS__)
#define parse_name(CTX, ...) parse_name(&CTX->parserContext, __VA_ARGS__)
#define parse_non_space(CTX, ...) parse_non_space(&CTX->parserContext, __VA_ARGS__)
#define parse_space_not_newline(CTX, ...) parse_space_not_newline(&CTX->parserContext, __VA_ARGS__)
#define parse_string(CTX, ...) parse_string(&CTX->parserContext, __VA_ARGS__)
#define parse_int(CTX, ...) parse_int(&CTX->parserContext, __VA_ARGS__)
#define parse_char(CTX, ...) parse_char(&CTX->parserContext, __VA_ARGS__)
#define parse_eof(CTX, ...) parse_eof(&CTX->parserContext, __VA_ARGS__)
#define count_lines(CTX, ...) get_location(&CTX->parserContext, __VA_ARGS__)
#define get_char(CTX, ...) get_char(&CTX->parserContext, __VA_ARGS__)
#define next_is_newline(CTX, ...) next_is_newline(&CTX->parserContext, __VA_ARGS__)



#define ERROR_SRC_RET(HEAD, FORMAT, ...) SourceLocation loc = count_lines(context, HEAD); error_src(loc, FORMAT, ##__VA_ARGS__); longjmp(context->jumpBuffer, 1)
#define ERROR_SRC(HEAD, FORMAT, ...) SourceLocation loc = count_lines(context, HEAD); error_src(loc, FORMAT, ##__VA_ARGS__);
#define LOG_SRC(HEAD, FORMAT, ...) SourceLocation loc = count_lines(context, HEAD); log_src(loc, FORMAT, ##__VA_ARGS__);


// void add_object() {

// }
// void add_label(Section* section) {
    
//     if (section->)
//     Assert(currentSection->labels_len < currentSection->labels_max);
//     Label* label = &currentSection->labels[currentSection->labels_len];
//     currentSection->labels_len++;
// }

Section* find_section(AssemblerContext* context, const char* name) {
    for (int si=0;si<context->sections_len;si++) {
        Section* section = &context->sections[si];
        if (equal(section->name, name)) {
            return section;
        }
    }
    return NULL;
}

bool find_label(AssemblerContext* context, const char* name, Section** out_section, Label** out_label) {
    for (int si=0;si<context->sections_len;si++) {
        Section* section = &context->sections[si];
        for (int li=0;li<section->labels_len;li++) {
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

LabelFixup* create_fixup(AssemblerContext* context, Instruction* inst, Operand* labelOperand) {
    Assert(context->labelFixups_len + 1 <= context->labelFixups_max);
    LabelFixup* fixup = &context->labelFixups[context->labelFixups_len];
    context->labelFixups_len++;

    Section* foundSection;
    Label* foundLabel;
    bool found = find_label(context, labelOperand->label.ptr, &foundSection, &foundLabel);
    if (!found) {
        ERROR_SRC_RET(inst->parseHead, "Label %s does not exist.\n", labelOperand->label.ptr);
    }

    uint64_t pc = builder_head(context->builder);
    
    int64_t relativeLow = foundLabel->estimated_addressLow - pc + labelOperand->immediate;
    int64_t relativeHigh = foundLabel->estimated_addressHigh - pc + labelOperand->immediate;

    // Because PC refers to the start of the instruction and not the end (since we haven't emitted it yet)
    // we add a margin of 4 bytes to make up for it. If any instruction with label has more than 4 bytes
    // we are in trouble. At the very least our label fixup code will cause an error when this happens.
    int margin = 8;

    if (abs(relativeLow) < 0x80-margin && abs(relativeHigh) < 0x80-margin) {
        fixup->reloc_size = 1;
    } else if (abs(relativeLow) < 0x8000-margin && abs(relativeHigh) < 0x8000-margin) {
        fixup->reloc_size = 2;
    } else {
        fixup->reloc_size = 4;
    }
    // printf("Create fixup '%s' pc=0x%zx relL=%zd relH=%zd rsize=%d\n", foundLabel->name.ptr, pc, relativeLow, relativeHigh, fixup->reloc_size);
    fixup->label = foundLabel;
    return fixup;
}

LabelFixup* create_fixup_abs(AssemblerContext* context, int head, uintptr_t address, string name, int size) {
    Assert(context->labelFixups_len + 1 <= context->labelFixups_max);
    LabelFixup* fixup = &context->labelFixups[context->labelFixups_len];
    context->labelFixups_len++;

    Section* foundSection;
    Label* foundLabel;
    bool found = find_label(context, name.ptr, &foundSection, &foundLabel);
    if (!found) {
        ERROR_SRC_RET(head, "Label %s does not exist.\n", name.ptr);
    }

    fixup->absolute = true;
    fixup->reloc_size = size;
    fixup->rom_offset = address;
    fixup->label = foundLabel;
    return fixup;
}

void prefix_namespace(string* name, const string namespace) {
    name->ptr = realloc(name->ptr, name->len + 1 + namespace.len + 1);
    Assert(name->ptr);
    memcpy(name->ptr + namespace.len + 1, name->ptr, name->len);
    memcpy(name->ptr, namespace.ptr, namespace.len);
    name->ptr[namespace.len] = '.';
    name->ptr[namespace.len + 1 + name->len] = '\0';
    name->len = namespace.len + 1 + name->len;
}

AssemblerError assemble(const char* in_text, size_t in_text_len, AssemblerOptions* options) {
    AssemblerContext* context = malloc(sizeof(AssemblerContext));
    memset(context, 0, sizeof(AssemblerContext));

    string out_text;
    bool preprocResult = preprocess_text(&context->parserContext, (string){ .len = in_text_len, .ptr = (char*)in_text }, &out_text, options->sourcePath);
    if (!preprocResult) {
        return LWM_ASM_ERROR_BAD;
    }

    writeFile("temp.pp", out_text.ptr, out_text.len);

    // for (int i=0;i<context->parserContext.spans_len;i++) {
    //     SourceSpan* loc = &context->parserContext.spans[i];
    //     printf("MAP %s %d %d\n", loc->file, loc->dst_start, loc->dst_end);
    // }

    const char* text = out_text.ptr;
    int text_len = out_text.len;

    // Memory leak. we never free context.

    context->source_file.ptr = (char*)options->sourcePath;
    context->parserContext.path = (char*)context->source_file.ptr;
    context->parserContext.text = (char*)text;
    context->parserContext.text_len = text_len;

    context->sections_len = 0;
    context->sections_max = 20;
    context->sections = malloc(sizeof(Section) * context->sections_max);
    memset(context->sections, 0, sizeof(Section) * context->sections_max);

    int jmpResult = setjmp(context->jumpBuffer);
    if (jmpResult != 0) {
        return LWM_ASM_ERROR_BAD;
    }

    {
        Assert(context->sections_len+1 <= context->sections_max);
        SourceLocation loc = count_lines(context, 0);
        Section* section = &context->sections[context->sections_len];
        section->addr = 0;
        section->location = loc;
        context->sections_len++;
        #define DEFAULT_SECTION_NAME "_default_"
        section->name = (string){ .ptr = DEFAULT_SECTION_NAME, .len = strlen(DEFAULT_SECTION_NAME) };

        section->labels_len = 0;
        section->labels_max = MAX_LABELS;
        section->objects_len = 0;
        section->objects_max = MAX_OBJECTS;
        section->labels = malloc(sizeof(Label) * section->labels_max);
        section->objects = malloc(sizeof(Object) * section->objects_max);
        memset(section->labels, 0, sizeof(Label) * section->labels_max);
        memset(section->objects, 0, sizeof(Object) * section->objects_max);
    }

    int head = 0;
    uint64_t estimated_addressLow = 0; // relative to block
    uint64_t estimated_addressHigh = 0; // relative to block

    uint8_t queuedAlignment = 0;

    string localNamespace = {.len = 0, .ptr = ""};

    while (true) {

        parse_space(context, &head);

        if(parse_eof(context, head)) {
            break;
        }

        int location_head = head;

        char dotChar = get_char(context, head);
        if (dotChar == '.') {
            if (localNamespace.len == 0) {
                ERROR_SRC_RET(head, "A local label must be preceded by a normal label.\n");
            }
            head++;
        }
        string name;
        int parsed_chars = parse_name(context, &head, &name);
        if (!parsed_chars) {
            char chr = get_char(context, head);
            ERROR_SRC_RET(head, "Unexpected character '%c'\n", chr);
        }

        if (dotChar == '.')  {
            prefix_namespace(&name, localNamespace);
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
            parse_space_not_newline(context, &head);


            bool dynamic_length = false;
            uint64_t array_length = 0;
            if(get_char(context, head) == '[') {
                head++;
                
                parse_space(context, &head);

                int loc_int_head = head;
                
                int parsed_chars = parse_int(context, &head, &array_length);
                if(!parsed_chars) {
                    dynamic_length = true;
                }
                
                // CHECK_EOF
                char chr = get_char(context, head);
                if(chr != ']') {
                    if(!parsed_chars) {
                        ERROR_SRC_RET(head, "Unexpected character '%c'\n",chr);
                    }
                }
                if(!dynamic_length && array_length == 0) {
                    ERROR_SRC_RET(loc_int_head, "Array length of zero is not allowed.\n");
                }
                if(array_length < 0) {
                    ERROR_SRC_RET(loc_int_head, "Array length cannot be negative! (%d)\n", (int)array_length);
                }
                    
                head++;
                parse_space_not_newline(context, &head);
            }
            
            if (array_length || dynamic_length) {
                int max_count = array_length <= 0 ? 100 : array_length;
                
                u8* bytes = malloc(max_count * data_bytes);
                memset(bytes, 0, max_count * data_bytes);

                LabelValue* labels = NULL;
                int labels_len = 0;
                int labels_max = 0;


                Section* currentSection = &context->sections[context->sections_len-1];

                Assert(currentSection->objects_len < currentSection->objects_max);
                Object* object = &currentSection->objects[currentSection->objects_len];
                memset(object, 0, sizeof(*object));
                currentSection->objects_len++;

                object->kind = OBJECT_DATA;
                object->alignment = queuedAlignment;
                queuedAlignment = 0;

                int elementCount = 0;
            
                while (true) {
                    if (elementCount == 0) {
                        parse_space_not_newline(context, &head);
                        if (next_is_newline(context, head)) {
                            if (dynamic_length) {
                                ERROR_SRC_RET(head, "Expected at least one value for dynamic array.\n.");
                            }
                            // Zeroed array
                            head++;
                            break;
                        }
                    } else {
                        parse_space(context, &head);
                    }

                    int gotString = false;

                    if (data_bytes == 1) {
                        string str;
                        parsed_chars = parse_string(context, &head, &str);
                        if (parsed_chars) {
                            if (elementCount + str.len > max_count) {
                                int newMax = max_count * 2 + 10;
                                void* newPtr = realloc(bytes, newMax * data_bytes);
                                Assert(newPtr);
                                memset((char*)newPtr + max_count * data_bytes, 0, (newMax - max_count) * data_bytes);
                                bytes = newPtr;
                                max_count = newMax;
                            }
                            memcpy(bytes + elementCount, str.ptr, str.len);
                            gotString = true;
                            elementCount += str.len;
                        } else {
                            // Not a string, check char and integers
                        }
                    }
                    
                    if (!gotString) {
                        uint64_t value;
                        parsed_chars = parse_int(context, &head, &value);
                        if(!parsed_chars) {
                            parsed_chars = parse_char(context, &head, (char*)&value);
                            if(!parsed_chars) {
                                string label;
                                parsed_chars = parse_name(context, &head, &label);
                                if(parsed_chars) {
                                    value = 0;
                                    if (!labels) {
                                        labels_max = 100;
                                        labels_len = 0;
                                        labels = calloc(1, sizeof(LabelValue) * labels_max);
                                    }
                                    Assert(labels_len < labels_max);
                                    labels[labels_len].label = label;
                                    labels[labels_len].offset = elementCount * data_bytes;
                                    labels_len++;
                                } else {
                                    char chr = get_char(context, head);
                                    ERROR_SRC_RET(head, "Expected integer or char, not '%c'\n.", chr);
                                }
                            }
                        }
                        if (elementCount + 1 > max_count) {
                            int newMax = max_count * 2 + 10;
                            void* newPtr = realloc(bytes, newMax * data_bytes);
                            Assert(newPtr);
                            memset((char*)newPtr + max_count * data_bytes, 0, (newMax - max_count) * data_bytes);
                            bytes = newPtr;
                            max_count = newMax;
                        }
                        memcpy(bytes + elementCount * data_bytes, &value, data_bytes);
                        elementCount++;
                    }

                    parse_space(context, &head);

                    char chr = get_char(context, head);
                    if(chr == ',') {
                        head++;
                        continue;
                    } else {
                        break;
                    }
                }
                if (!dynamic_length && array_length != elementCount && elementCount != 0) {
                    ERROR_SRC_RET(head, "Array length and element count mismatch! (%d != %d)\n.", (int)array_length, elementCount);
                }
                if (dynamic_length) {
                    array_length = elementCount;
                }
                
                object->dataobj.bytes = bytes;
                object->dataobj.size = array_length * data_bytes;
                object->dataobj.elementSize = data_bytes;
                object->dataobj.labels = labels;
                object->dataobj.labels_len = labels_len;
            } else {
                array_length = 1;

                int prev_head = head;
                uint64_t value;
                u8* bytes = NULL;
                LabelValue* labels = NULL;
                int parsed_chars = parse_int(context, &head, &value);
                if(!parsed_chars) {
                    parsed_chars = parse_char(context, &head, (char*)&value);
                    if(!parsed_chars) {
                        string label;
                        parsed_chars = parse_name(context, &head, &label);
                        if(parsed_chars) {
                            labels = calloc(1, sizeof(LabelValue));
                            labels->label = label;
                            labels->offset = 0;
                        }
                    }
                }
                if(parsed_chars && !labels) {
                    bytes = malloc(data_bytes);
                    memset(bytes, 0, data_bytes);
                    memcpy(bytes, &value, data_bytes);
                }


                Section* currentSection = &context->sections[context->sections_len-1];
                Assert(currentSection->objects_len < currentSection->objects_max);
                Object* object = &currentSection->objects[currentSection->objects_len];
                memset(object, 0, sizeof(*object));
                currentSection->objects_len++;

                object->kind = OBJECT_DATA;
                object->alignment = queuedAlignment;
                queuedAlignment = 0;
                object->dataobj.bytes = bytes;
                object->dataobj.size = data_bytes;
                object->dataobj.elementSize = data_bytes;
                if (labels) {
                    object->dataobj.labels_len = 1;
                    object->dataobj.labels = labels;
                }
            }

            estimated_addressLow += data_bytes * array_length;
            estimated_addressHigh += data_bytes * array_length;
            continue;
        }

        if (equal(name, "section")) {
            // origin, specify address for following labels and instructions
            parse_space(context, &head);

            string sectionName;
            char chr = get_char(context, head);
            if (chr == '"') {
                parsed_chars = parse_string(context, &head, &sectionName);
                if (!parsed_chars) {
                    ERROR_SRC_RET(head, "Could not parse section name.\n");
                }
            } else if (chr == '\'') {
                ERROR_SRC_RET(head, "Section name cannot be specified with single quote.\n");
            } else {
                parsed_chars = parse_non_space(context, &head, &sectionName);
                if (!parsed_chars) {
                    ERROR_SRC_RET(head, "Could not parse section name, '%c'.\n", chr);
                }
            }

            parse_space(context, &head);

            
            uint64_t addr;
            int parsed_chars = parse_int(context, &head, &addr);
            if(parsed_chars) {
                context->sections[context->sections_len-1].estimatedAddress_low = estimated_addressLow;
                context->sections[context->sections_len-1].estimatedAddress_high = estimated_addressHigh;

                // Create new block and set its address
                Assert(context->sections_len+1 <= context->sections_max);
                SourceLocation loc = count_lines(context, head);
                Section* section = &context->sections[context->sections_len];
                section->location = loc;
                context->sections_len++;
                
                section->addr = addr;
                section->name = sectionName;
                section->labels_len = 0;
                section->labels_max = MAX_LABELS;
                section->objects_len = 0;
                section->objects_max = MAX_OBJECTS;
                section->labels = malloc(sizeof(Label) * section->labels_max);
                section->objects = malloc(sizeof(Object) * section->objects_max);
                memset(section->labels, 0, sizeof(Label) * section->labels_max);
                memset(section->objects, 0, sizeof(Object) * section->objects_max);

                estimated_addressLow = 0;
                estimated_addressHigh = 0;
            } else {
                Section* section = find_section(context, sectionName.ptr);
                if (!section) {
                    ERROR_SRC_RET(head, "Section '%s' does not exist. Define section with 'section \".text\" 0x0'.\n", sectionName.ptr);
                }

                estimated_addressLow = section->estimatedAddress_low;
                estimated_addressHigh = section->estimatedAddress_high;
            }
            
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
            if(alignment == 0 || alignment == 1) {
                ERROR_SRC_RET(head, "Alignment by zero or one is pointless, was it a mistake?\n");
            }

            queuedAlignment = 32 - lzcnt(alignment)-1;

            estimated_addressLow += 0;
            estimated_addressHigh += alignment-1;

            continue;
        }

        if (!parsed_chars) {
            ERROR_SRC_RET(head, "Unexpected character '%c'\n", get_char(context, head));
        }

        parse_space_not_newline(context, &head);

        if (get_char(context, head) == ':') {
            if (dotChar != '.') {
                localNamespace = name;
            }
            // label
            Section* currentSection = &context->sections[context->sections_len-1];
            // if (currentSection->labels_len >= MAX_LABELS_PER_BLOCK) {
            //     ERROR_SRC_RET(location_head, "Max label limit reached! (%d)\n", MAX_LABELS_PER_BLOCK);
            // }

            Section* sec;
            Label* lab;
            bool exists = find_label(context, name.ptr, &sec, &lab);
            if (exists) {
                ERROR_SRC_RET(head, "Label '%s' already exists\n", name.ptr);
            }

            head++;

            Assert(currentSection->labels_len < currentSection->labels_max);
            Label* label = &currentSection->labels[currentSection->labels_len];
            currentSection->labels_len++;

            label->estimated_addressLow = estimated_addressLow;
            label->estimated_addressHigh = estimated_addressHigh;
            label->name = name;
            label->object_id = currentSection->objects_len; // Refer to next object
            continue;
        }

        Instruction inst = {0};
        inst.parseHead = location_head;

        int operandIndex = 0;

        // Parse operands (if no newline and not EOF)
        if (!next_is_newline(context, head) && !parse_eof(context, head)) {
        while (true) {
            string operand;
            uint64_t value;

            char dotChar = get_char(context, head);
            if (dotChar == '.') {
                if (localNamespace.len == 0) {
                    ERROR_SRC_RET(head, "A local label must be preceded by a normal label.\n");
                }
                head++;
            }

            int parsed_chars = parse_name(context, &head, &operand);
            if(parsed_chars) {
                int reg = regname_to_number(operand);
                if(reg != INVALID_REGISTER) {
                    inst.operands[operandIndex].regnum = reg;
                    operandIndex++;
                } else {
                    if (dotChar == '.')  {
                        prefix_namespace(&operand, localNamespace);
                    }
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
                char chr = get_char(context, head);
                if (chr == '[') {
                    // Parse memory addressing
                    //   [base + index + displacement]
                    // displacement can be made up of one label and multiple integers that are added
                    head++;
                    
                    Operand* memoryOperand = &inst.operands[operandIndex];
                    operandIndex++;

                    int parsedRegs = 0;

                    bool expectOperator = false;
                    bool expectLiteral = false;
                    bool prefixedWithMinusOperator = false;

                    while (true) {

                        parse_space(context, &head);

                        if (parse_eof(context, head)) {
                            ERROR_SRC_RET(head, "Expected ] to end memory operand, got end of file.\n");
                        }

                        char chr = get_char(context, head);
                        if (chr == ']') {
                            // @TODO Empty addressing is not allowed. Currently it becomes ABS16.
                            head++;
                            break;
                        }

                        if (!expectOperator) {
                            string word;
                            uint64_t disp;
                            int parsed_chars = 0;
                            if (!expectLiteral) {
                                parsed_chars = parse_name(context, &head, &word);
                            }
                            if(parsed_chars) {
                                int reg = regname_to_number(word);
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
                                    if (memoryOperand->label.len) {
                                        ERROR_SRC_RET(head, "Memory addressing is limited to one label.\n");
                                    } else {
                                        memoryOperand->label = word;
                                    }
                                }
                            } else {
                                parsed_chars = parse_int(context, &head, &value);
                                if (parsed_chars) {
                                    if (prefixedWithMinusOperator) {
                                        memoryOperand->immediate -= value;
                                    } else {
                                        memoryOperand->immediate += value;
                                    }
                                } else {
                                    char chrValue;
                                    parsed_chars = parse_char(context, &head, &chrValue);
                                    if (parsed_chars) {
                                        if (prefixedWithMinusOperator) {
                                            memoryOperand->immediate -= chrValue;
                                        } else {
                                            memoryOperand->immediate += chrValue;
                                        }
                                    } else {
                                        char chr = get_char(context, head);
                                        ERROR_SRC_RET(head, "Bad character in memory addressing '%c'.\n", chr);
                                    }
                                }
                                expectLiteral = false;
                            }
                            expectOperator = true;
                        } else {
                            // parse_space(context, &head);
                            chr = get_char(context, head);
                            if (chr == '+') {
                                head++;
                            } else if (chr == '-') {
                                head++;
                                expectLiteral = true;
                                prefixedWithMinusOperator = true;
                            } else {
                                ERROR_SRC_RET(head, "Bad character in memory addressing '%c'.\n", chr);
                            }
                            expectOperator = false;
                        }
                    }

                    if (memoryOperand->label.len) {
                        if (parsedRegs == 0) {
                            // Pick largest displacement since we don't know how far away the label is.
                            memoryOperand->form = ADDRESSING_PC_DISP32;
                        } else if (parsedRegs == 1) {
                            memoryOperand->form = ADDRESSING_REG1_PC_DISP32;
                        } else if (parsedRegs == 2) {
                            ERROR_SRC_RET(head, "Memory addressing does not support 2 regs with labels.\n");
                        }
                    } else {
                        int bits = 32 - lzcnt(abs(memoryOperand->immediate));
                        if (parsedRegs == 0) {
                            if (bits <= 15)
                                memoryOperand->form = ADDRESSING_ABS16;
                            else if (bits <= 31)
                                memoryOperand->form = ADDRESSING_ABS32;
                            else
                                memoryOperand->form = ADDRESSING_ABS64;
                        } else if (parsedRegs == 1) {
                            if (bits <= 7)
                                memoryOperand->form = ADDRESSING_REG1_DISP8;
                            else if (bits <= 15)
                                memoryOperand->form = ADDRESSING_REG1_DISP16;
                            else if (bits <= 31)
                                memoryOperand->form = ADDRESSING_REG1_DISP32;
                            else
                                memoryOperand->form = ADDRESSING_REG1_DISP64;
                        } else if (parsedRegs == 2) {
                            if (bits <= 7)
                                memoryOperand->form = ADDRESSING_REG2_DISP8;
                            else if (bits <= 15)
                                memoryOperand->form = ADDRESSING_REG2_DISP16;
                            else if (bits <= 31)
                                memoryOperand->form = ADDRESSING_REG2_DISP32;
                            else
                                memoryOperand->form = ADDRESSING_REG2_DISP64;
                        }
                    }

                } else {
                    ERROR_SRC_RET(location_head, "Could not parse operands. '%c', %s\n",  chr, name.ptr);
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

        // Pseudo instructions
        if (equal(name, "mov")) {
            name = STRING("or");
            inst.operands[2].regnum = inst.operands[1].regnum;
        }
        else if (equal(name, "hlt")) {
            name = STRING("stb");
            inst.operands[0].regnum = 0;
            inst.operands[1].form = ADDRESSING_ABS16;
            inst.operands[1].immediate = 0xF000;
        } else if (equal(name, "li")) {
            if (inst.operands[1].immediate >> 63) {
                name = STRING("lis");
            }
        }

        int minBytes, maxBytes;
        bool decoded = prepare_encode_inst(name, &inst, &minBytes, &maxBytes);
        if (!decoded) {
            ERROR_SRC_RET(location_head, "Unknown instruction '%s'\n", name.ptr);
        }

        
        

        Section* currentSection = &context->sections[context->sections_len-1];
        Assert(currentSection->objects_len < currentSection->objects_max);
        Object* object = &currentSection->objects[currentSection->objects_len];
        currentSection->objects_len++;

        object->kind = OBJECT_INSTRUCTION;
        object->alignment = queuedAlignment;
        queuedAlignment = 0;
        object->inst = inst;

        estimated_addressLow += minBytes;
        estimated_addressHigh += maxBytes;
    }
    
    context->sections[context->sections_len-1].estimatedAddress_low = estimated_addressLow;
    context->sections[context->sections_len-1].estimatedAddress_high = estimated_addressHigh;
    
    
    //###########################
    //   CONSTRUCT THE BINARY
    //###########################

    // Sort blocks from low to high address.
    int section_len = context->sections_len;
    int* sorted_section_indices = malloc(section_len * sizeof(int));
    for(int i=0;i<section_len;i++) {
        sorted_section_indices[i] = i;
    }
    for(int j=0;j<section_len;j++) {
        bool changed = false;
        for(int i=0;i<section_len-1;i++) {
            Section* a = &context->sections[sorted_section_indices[i]];
            Section* b = &context->sections[sorted_section_indices[i+1]];
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
    // and estimated worst-case ROM size.
    uint64_t rom_max = 0;
    for(int i=0;i<section_len;i++) {
        Section* section = &context->sections[i];
        for(int j=0;j<section->labels_len;j++) {
            Label* label = &section->labels[j];
            label->estimated_addressLow += section->addr;
            label->estimated_addressHigh += section->addr;
            // printf("Label %s, 0x%zx 0x%zx\n", label->name.ptr, label->estimated_addressLow, label->estimated_addressHigh);
        }
        uint64_t newRomSize = section->addr + section->estimatedAddress_high;
        if (newRomSize > rom_max) {
            rom_max = newRomSize;
        }
    }
    
    // printf("Estimated sections:\n");
    // for(int i=0;i<section_len;i++) {
    //     Section* a = &context->sections[sorted_section_indices[i]];
    //     printf(" %s [0x%zx - 0x%zx] %d\n", a->name.ptr, a->addr, a->size, sorted_section_indices[i]);
    // }

    // Emit sections to binary
    Section* last_section = &context->sections[sorted_section_indices[section_len-1]];
    uint8_t* rom_ptr = malloc(rom_max);
    uint64_t rom_len = 0;
    memset(rom_ptr, 0, rom_max);

    context->labelFixups_len = 0;
    context->labelFixups_max = 1000;
    context->labelFixups = malloc(sizeof(LabelFixup) * context->labelFixups_max);

    Builder  _builder = {0};
    Builder* builder = &_builder;
    context->builder = builder;

    builder->context = context;
    builder->create_fixup = (FN_create_fixup)create_fixup;

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
            while (checkLabelIndex < section->labels_len) {
                Label* label = &section->labels[checkLabelIndex];
                if (label->object_id != oi) {
                    break;
                }
                label->final_address = builder_head(builder);
                checkLabelIndex++;
                if (options->verbose) {
                    printf("Update label %s 0x%zx obj=%d secaddr=0x%zx\n", label->name.ptr, label->final_address, label->object_id, section->addr);
                }
            }

            if (object->kind == OBJECT_DATA) {
                Assert(object->dataobj.size != 0);
                uintptr_t final_address = builder_head(builder);
                if (object->dataobj.bytes) {
                    emit_bytes(builder, object->dataobj.bytes, object->dataobj.size);
                } else {
                    emit_zeros(builder, object->dataobj.size);
                }
                for (int li=0;li<object->dataobj.labels_len;li++) {
                    LabelValue* label = &object->dataobj.labels[li];
                    uintptr_t addr = final_address + label->offset;
                    create_fixup_abs(context, 0, addr, label->label, object->dataobj.elementSize);
                }
                continue;
            }
            char message[512];

            Instruction* inst = &object->inst;
            bool encoded = encode_inst(builder, inst, message);
            if (!encoded) {
                ERROR_SRC_RET(inst->parseHead, "%s", message);
            }

        }
        // Update last addresses
        while (checkLabelIndex < section->labels_len) {
            Label* label = &section->labels[checkLabelIndex];
            if (label->object_id != section->objects_len) {
                // printf("Not %s %d %d\n", label->name.ptr, label->object_id, section->objects_len);
                Assert(label->object_id == section->objects_len);
            }
            
            label->final_address = builder_head(builder);
            checkLabelIndex++;
            if (options->verbose) {
                printf("Update label %s 0x%zx obj=%d secaddr=0x%zx\n", label->name.ptr, label->final_address, label->object_id, section->addr);
            }
        }
        
        section->size = builder->byteStream_len - section->addr;

        size_t rom_stream_offset = builder->byteStream - rom_ptr;
        size_t rom_stream_length = builder->byteStream_len;
        if (rom_stream_offset + rom_stream_length > rom_len)
            rom_len = rom_stream_offset + rom_stream_length;
    }

    // for(int i=0;i<section_len;i++) {
    //     Section* section = &context->sections[i];
    //     for(int j=0;j<section->labels_len;j++) {
    //         Label* label = &section->labels[j];
    //         printf("Label %s, 0x%zx\n", label->name.ptr, label->final_address);
    //     }
    // }
    
    if (options->verbose) {
        printf("Estimated sections:\n");
    }
    for(int i=0;i<section_len;i++) {
        Section* a = &context->sections[sorted_section_indices[i]];
        if (options->verbose) {
            printf(" %s [0x%zx - 0x%zx] %d\n", a->name.ptr, a->addr, a->size, sorted_section_indices[i]);
        }

        if(i+1 >= section_len)
            break;

        // Check section overlaps
        Section* b = &context->sections[sorted_section_indices[i+1]];
        if(a->addr + a->size > b->addr) {
            error("Section overlap!\n");
            printf("  %s:%d - %s [0x%zx - 0x%zx]\n", a->location.file, a->location.line, a->name.ptr, a->addr, a->addr + a->size);
            printf("  %s:%d - %s [0x%zx - 0x%zx]\n", b->location.file, b->location.line, b->name.ptr, b->addr, b->addr + b->size);
            longjmp(context->jumpBuffer, 1);
        }
    }

    for (int li=0;li<context->labelFixups_len;li++) {
        LabelFixup* fixup = &context->labelFixups[li];

        if (!fixup->absolute) {
            uint64_t final_address = fixup->label->final_address;
            if (fixup->reloc_size == 1) {
                final_address += *(int8_t*)(rom_ptr + fixup->rom_offset);
            } else if (fixup->reloc_size == 2) {
                final_address += *(int16_t*)(rom_ptr + fixup->rom_offset);
            } else if (fixup->reloc_size == 4) {
                final_address += *(int32_t*)(rom_ptr + fixup->rom_offset);
            }
            int64_t relative = final_address - (fixup->rom_offset + fixup->reloc_size);
            if (options->verbose) {
                printf("Fixup%d *0x%zx += 0x%zx (final 0x%zx)\n",fixup->reloc_size*8, fixup->rom_offset, relative, final_address);
            }
            if (fixup->reloc_size == 1) {
                if (abs(relative) >= 0x80) {
                    error("Label '%s' fixup too big for rel%d (value %zd)\n", fixup->label->name.ptr, fixup->reloc_size*8, relative);
                    longjmp(context->jumpBuffer, 1);
                }
                *(int8_t*)(rom_ptr + fixup->rom_offset) = relative;
            } else if (fixup->reloc_size == 2) {
                if (abs(relative) >= 0x8000) {
                    error("Label '%s' fixup too big for rel%d (value %zd)\n", fixup->label->name.ptr, fixup->reloc_size*8, relative);
                    longjmp(context->jumpBuffer, 1);
                }
                *(int16_t*)(rom_ptr + fixup->rom_offset) = relative;
            } else {
                
                *(int32_t*)(rom_ptr + fixup->rom_offset) = relative;
            }
            continue;
        }

        uintptr_t abs_address = fixup->label->final_address;
        if (fixup->reloc_size == 1) {
            abs_address += *(int8_t*)(rom_ptr + fixup->rom_offset);
        } else if (fixup->reloc_size == 2) {
            abs_address += *(int16_t*)(rom_ptr + fixup->rom_offset);
        } else if (fixup->reloc_size == 4) {
            abs_address += *(int32_t*)(rom_ptr + fixup->rom_offset);
        }
        if (options->verbose) {
            printf("Fixup%d *0x%zx = 0x%zx\n", fixup->reloc_size*8, fixup->rom_offset, abs_address);
        }
        if (fixup->reloc_size == 1) {
            if (abs_address > 0xFF) {
                error("Label '%s' fixup too big for abs%d (value 0x%zx)\n", fixup->label->name.ptr, fixup->reloc_size*8, abs_address);
                longjmp(context->jumpBuffer, 1);
            }
            *(int8_t*)(rom_ptr + fixup->rom_offset) = abs_address;
        } else if (fixup->reloc_size == 2) {
            if (abs_address > 0xFFFF) {
                error("Label '%s' fixup too big for abs%d (value 0x%zx)\n", fixup->label->name.ptr, fixup->reloc_size*8, abs_address);
                longjmp(context->jumpBuffer, 1);
            }
            *(int16_t*)(rom_ptr + fixup->rom_offset) = abs_address;
        } else {
            
            *(int32_t*)(rom_ptr + fixup->rom_offset) = abs_address;
        }
    }

    options->rom = rom_ptr;
    options->rom_len = rom_len;
    options->rom_max = rom_max;

    return LWM_ASM_ERROR_NONE;

}


