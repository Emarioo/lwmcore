#include "lwmcore/assembler.h"

//#######################
//   PUBLIC FUNCTIONS
//#######################

#define ERROR_SRC_RET(HEAD, FORMAT, ...) Location loc = count_lines(&text, HEAD, info->source_file.ptr); error_src(loc, FORMAT, ##__VA_ARGS__); return 1
#define ERROR_SRC(HEAD, FORMAT, ...) Location loc = count_lines(&text, HEAD, info->source_file.ptr); error_src(loc, FORMAT, ##__VA_ARGS__);
#define LOG_SRC(HEAD, FORMAT, ...) Location loc = count_lines(&text, HEAD, info->source_file.ptr); log_src(loc, FORMAT, ##__VA_ARGS__);

bool at_eof(string* text, int* head) {
    return *head >= text->len;
}

bool assemble(string text, Bin* bin, AssemblerInfo* info) {

    /*
        1. Define a specification for the binary output (array of instructions?).
        2. Define a specification for the assembly syntax (with implementation in Logic World in mind).
        3. Implement parsing of characters, identifiers, numbers. Construct an intermediate list struct of instructions
           and in a second phase convert it to binary output format.
    */

    // Apply include directives
    {
        // TODO: You can do more efficient memcpys here but computers are fast so it's honestly fine.
        int buffer_max = 0x100000;
        if(text.len > buffer_max)
            buffer_max = text.len;
        string buffer;
        buffer.ptr = malloc(buffer_max); // 1 MB
        Assert(buffer.ptr);
        buffer.len = text.len;
        memcpy(buffer.ptr, text.ptr, text.len);
        
        int head = 0;
        while(head < buffer.len) {
            // quick check
            if(buffer.ptr[head++] != 'i') {
                continue;
            }

            int head_before_include = head - 1;

            int keyword_len = 6;
            if(buffer.len - head < keyword_len || strncmp(buffer.ptr + head, "nclude", keyword_len)) {
                continue;
            }
            head += keyword_len;

            parse_space(&buffer, &head);

            int head_file_path = head;
            string file_path;
            int parsed_chars = parse_string(&buffer, &head, &file_path);
            if(!parsed_chars) {
                ERROR_SRC_RET(head, "Expected a string!\n");
            }

            FILE* file = fopen(file_path.ptr, "rb");
            if(!file) {
                ERROR_SRC_RET(head_file_path, "Could not read '%s'.\n", file_path.ptr);
            }
            fseek(file, 0, SEEK_END);
            int filesize = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            // ensure we have enough space
            if(filesize + buffer.len < buffer_max) {
                int new_max = (buffer_max + filesize) * 2;
                void* new_buffer = realloc(buffer.ptr, new_max);
                Assert(new_buffer);
                buffer.ptr = new_buffer;
                buffer_max = new_max;
            }

            /*
                Our buffer looks like this:
                    [ parsed_chars | include | remaining_chars ]
                We want to insert the file data after parsed chars and replace the include directive like this:
                    [ parsed_chars | filedata | remaining_chars ]
            */
            // Move remaining_chars to the right to make space for file data
            memmove(buffer.ptr + head_before_include + filesize, buffer.ptr + head, buffer.len - head);
            // Replace include, insert file data and adjust the len and head fields/variables accordingly.
            fread(buffer.ptr + head_before_include, 1, filesize, file);
            buffer.len += filesize - (head - head_before_include);
            head = head_before_include;

            fclose(file);
        }

        // TODO: Memory leak of buffer
        text = buffer;
    }

    // Parse the stuff

    {
        Location loc = count_lines(&text, 0, info->source_file.ptr);
        Block* block = &info->blocks[info->block_len];
        block->addr = 0;
        block->location = loc;
        info->block_len++;
    }

    // TODO: Handle end of file
    #define CHECK_EOF if (at_eof(&text, &head)) { Location loc = count_lines(&text, head, info->source_file.ptr); error_src(loc, "Unexpected end of file!\n"); return 1; };

    int relative_address = 0;
    int head = 0;
    string name;
    while (head < text.len) {
        parse_space(&text, &head);

        if(head >= text.len) {
            break;
        }

        int location_head = head;

        int parsed_chars = parse_name(&text, &head, &name);
        if(!parsed_chars) {
            ERROR_SRC_RET(location_head, "Unexpected character '%c'\n", text.ptr[head]);
        }

        int data_bytes = 0;
        bool may_be_string = false;
        if(equal(name, "char")) {
            data_bytes = 1;
            may_be_string = true;
        } else if(equal(name, "int8")) {
            data_bytes = 1;
        } else if(equal(name, "int16")) {
            data_bytes = 2;
        }
        if(data_bytes) {
            parse_space(&text, &head);
            
            CHECK_EOF
            
            bool is_dynamic_string = false; // dynamic as in variable length, not heap allocated

            // parse optional array

            int array_length = 0;
            if(text.ptr[head] == '[') {
                head++;
                
                parse_space(&text, &head);

                int loc_int_head = head;
                
                int parsed_chars = parse_int(&text, &head, &array_length);
                
                CHECK_EOF
                
                if(text.ptr[head] != ']') {
                    if(!parsed_chars) {
                        ERROR_SRC_RET(head, "Unexpected character '%c'\n", text.ptr[head]);
                    } else if(!may_be_string) {
                        ERROR_SRC_RET(head, "Unexpected character '%c'\n", text.ptr[head]);
                    }
                }
                if(!parsed_chars && may_be_string) {
                    is_dynamic_string = true;
                }
                if(parsed_chars && array_length == 0) {
                    ERROR_SRC_RET(loc_int_head, "Array length of zero is not allowed.\n");
                }
                if(array_length < 0) {
                    ERROR_SRC_RET(loc_int_head, "Array length cannot be negative! (%d)\n", array_length);
                }
                    
                head++;
            }

            parse_space(&text, &head);

            // parse optional values
            
            // TODO: Where do we put initializer values?
            // TODO: Verify that the values fit in the arrays
            if(is_dynamic_string || (may_be_string && array_length)) {
                int prev_head = head;
                string str;
                int parsed_chars = parse_string(&text, &head, &str);
                if(parsed_chars) {
                    LOG_SRC(prev_head, "Emit \"%s\"\n", str.ptr);
                } else {
                    ERROR_SRC_RET(head, "Dynamic string variables require a string literal. The size is determined using the literal\n.", text.ptr[head]);
                }
            } else if(array_length > 0) {
                int initializer_head = head;
                if(text.ptr[head] != '{') {
                    ERROR_SRC_RET(head, "Expected curly brace for array initializer, not '%c'\n.", text.ptr[head]);
                }
                head++;
                int temp_len = 0;
                int temp[100];
                while (head < text.len) {
                    parse_space(&text, &head);
                    
                    if(text.ptr[head] == '}') {
                        head++;
                        break;
                    }

                    int value = 0;
                    int parsed_chars = parse_int(&text, &head, &value);
                    if(!parsed_chars) {
                        ERROR_SRC_RET(head, "Expected integer, not '%c'\n.", text.ptr[head]);
                    }
                    temp[temp_len++] = value;

                    parse_space(&text, &head);

                    if(text.ptr[head] == ',') {
                        head++;
                        continue;
                    }
                }

                LOG_SRC(initializer_head, "Emit { ");
                for(int i=0;i<temp_len;i++) {
                    if(i!=0)
                        printf(", ");
                    printf("%d", temp[i]);
                }
                printf("}\n");
            } else {
                int prev_head = head;
                int value;
                int parsed_chars = parse_int(&text, &head, &value);
                if(parsed_chars) {
                    LOG_SRC(prev_head, "Emit %d\n", value);
                }
            }
            relative_address += data_bytes * array_length;
            continue;
        }

        if (equal(name, "ORG")) {
            // origin, specify address for following labels and instructions
            parse_space(&text, &head);
            
            int addr;
            int parsed_chars = parse_int(&text, &head, &addr);
            if(!parsed_chars) {
                Location loc = count_lines(&text, head, info->source_file.ptr);
                error_src(loc, "Unexpected character '%c'\n", text.ptr[head]);
                return 1;
            }
            
            // Determine size for previous block
            info->blocks[info->block_len-1].size = relative_address;

            // Create new block and set its address
            Location loc = count_lines(&text, head, info->source_file.ptr);
            Block* block = &info->blocks[info->block_len];
            block->addr = addr;
            block->location = loc;
            info->block_len++;
            continue;
        }

        if (!parsed_chars) {
            Location loc = count_lines(&text, head, info->source_file.ptr);
            error_src(loc, "Unexpected character '%c'\n", text.ptr[head]);
            return false;
        }

        parse_space(&text, &head);

        if (text.ptr[head] == ':') {
            // label
            Block* block = &info->blocks[info->block_len-1];
            if (block->label_len >= MAX_LABELS) {
                Location loc = count_lines(&text, location_head, info->source_file.ptr);
                error_src(loc, "Max label limit reached! (%d)\n", MAX_LABELS);
                return 1;
            }

            // TODO: Alignment

            head++;
            block->labels[block->label_len].addr = relative_address;
            block->labels[block->label_len].name = name;
            block->label_len++;
        } else {
            // instruction

            Instruction inst = {0};
            inst.name = name;

            string operand0;
            int parsed_chars = parse_name(&text, &head, &operand0);
            if(!parsed_chars) {
                ERROR_SRC(head, "Could not parse instruction. Skipping to next line.\n");
                skip_line(&text, &head);
                continue;
            }

            inst.reg0 = get_regnr(operand0);
            if(inst.reg0 == -1) {
                ERROR_SRC(head, "Could not parse instruction. Skipping to next line.\n");
                skip_line(&text, &head);
                continue;
            }
            
            parse_space(&text, &head);
            
            if(text.ptr[head] != ',') {
                continue;
            }
            head++;

            parse_space(&text, &head);

            string operand1;
            int value;
            parsed_chars = parse_name(&text, &head, &operand1);
            if(parsed_chars) {
                inst.reg1 = get_regnr(operand1);
                if(inst.reg1 == -1) {
                    ERROR_SRC(head, "Could not parse instruction. Skipping to next line.\n");
                    skip_line(&text, &head);
                    continue;
                }
            } else {
                parsed_chars = parse_int(&text, &head, &value);
                if (parsed_chars) {
                    inst.imm = value;
                }
            }
            if(!parsed_chars) {
                ERROR_SRC(head, "Could not parse instruction. Skipping to next line.\n");
                skip_line(&text, &head);
                continue;
            }
            
            Block* block = &info->blocks[info->block_len-1];
            if(block->inst_len >= MAX_INST_PER_BLOCK) {
                ERROR_SRC_RET(head, "Max instruction per block reached! (%d)\n", MAX_INST_PER_BLOCK);
            }
            block->insts[block->inst_len++] = inst;
            relative_address+=2;
        } 
    }
    // Determine size for previous block
    info->blocks[info->block_len-1].size = relative_address;

    //###########################
    //   CONSTRUCT THE BINARY
    //###########################

    // Sort blocks from low to high address.
    int block_len = info->block_len;
    int* sorted_block_indices = malloc(block_len);
    for(int i=0;i<block_len;i++) {
        sorted_block_indices[i] = i;
    }
    for(int j=0;j<block_len;j++) {
        bool changed = false;
        for(int i=0;i<block_len - j - 1;i++) {
            Block* a = &info->blocks[i];
            Block* b = &info->blocks[i+1];
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
        Block* block = &info->blocks[i];
        for(int j=0;j<block->label_len;j++) {
            Label* label = &block->labels[j];
            label->addr += block->addr;
        }
    }

    // Check block overlaps
    bool has_overlap = false;
    for(int i=0;i<block_len-1;i++) {
        Block* a = &info->blocks[sorted_block_indices[i]];
        Block* b = &info->blocks[sorted_block_indices[i+1]];

        if(a->addr + a->size > b->addr) {
            error("Block overlap!\n");
            printf("  %s:%d - [0x%x - 0x%x]\n", a->location.file, a->location.line, a->addr, a->addr + a->size);
            printf("  %s:%d - [0x%x - 0x%x]\n", b->location.file, b->location.line, b->addr, b->addr + b->size);
            has_overlap = true;
        }
    }
    if(has_overlap)
        return false;

    return true;
} 

//#######################
//   PRIVATE FUNCTIONS
//#######################

int parse_name(string* text, int* head, string* name) {
    name->len = 0;
    name->ptr = nullptr;
    int origin_head = *head;

    char chr = text->ptr[*head];
    if (!(((chr|32) >= 'a' && (chr|32) <= 'z') || chr == '_')) {
        return 0;
    }
    name->ptr = &text->ptr[*head];
    while(*head < text->len) {
        chr = text->ptr[*head];
        if (((chr|32) >= 'a' && (chr|32) <= 'z') || (chr >= '0' && chr <= '9') || chr == '_') {
            ++*head;
            name->len++;
            continue;
        }
        break;
    }
    // TODO: Memory leak, we allocate new string to null terminate it. Other characters how up otherwise.
    char* buf = (char*)malloc(name->len + 1);
    memcpy(buf, name->ptr, name->len);
    buf[name->len] = '\0';
    name->ptr = buf;
    return *head - origin_head;
}

int parse_int(string* text, int* head, int* value) {
    *value = 0;
    int origin_head = *head;

    bool negate = false;
    if(text->ptr[*head] == '-') {
        negate = true;
        ++*head;
    }

    if (*head + 2 < text->len && text->ptr[*head] == '0' && text->ptr[*head+1] == 'x') {
        // 16-base hexidecimal
        *head += 2;
        while(*head < text->len) {
            char chr = text->ptr[*head];
            if(chr == '_') {
                ++*head;
                continue;
            }
            if (chr >= '0' && chr <= '9') {
                ++*head;
                *value = 16 * *value + chr - '0';
                continue;
            }
            if ((chr|32) >= 'a' && (chr|32) <= 'f') {
                ++*head;
                *value = 16 * *value + chr - 'a';
                continue;
            }
            break;
        }
     } else if (*head + 2 < text->len && text->ptr[*head] == '0' && text->ptr[*head+1] == 'b') {
        // 2-base hexidecimal
        *head += 2;
        while(*head < text->len) {
            char chr = text->ptr[*head];
            if(chr == '_') {
                ++*head;
                continue;
            }
            if (chr >= '0' && chr <= '1') {
                ++*head;
                *value = 2 * *value + chr - '0';
                continue;
            }
            break;
        }
    } else if (*head + 2 < text->len && text->ptr[*head] == '0' && text->ptr[*head+1] == 'o') {
        // 2-base hexidecimal
        *head += 2;
        while(*head < text->len) {
            char chr = text->ptr[*head];
            if(chr == '_') {
                ++*head;
                continue;
            }
            if (chr >= '0' && chr <= '7') {
                ++*head;
                *value = 8 * *value + chr - '0';
                continue;
            }
            break;
        }
    } else {
        // 10-base integer
        while(*head < text->len) {
            char chr = text->ptr[*head];
            if(chr == '_') {
                ++*head;
                continue;
            }
            if (chr >= '0' && chr <= '9') {
                ++*head;
                *value = 10 * *value + chr - '0';
                continue;
            }
            break;
        }
    }
    if(negate)
        *value = -*value;
    return *head - origin_head;
}

int parse_space(string* text, int* head) {
    int origin_head = *head;

    while(*head < text->len) {
        char chr = text->ptr[*head];
        char chr_next = 0;
        if (*head + 1 < text->len)
            chr_next = text->ptr[*head + 1];

        if(chr == '"' || chr == '\'') {
            char end_chr = chr;
            ++*head;
            while(*head < text->len) {
                // TODO: Handle escaped characters
                if(text->ptr[*head] == end_chr)
                    break;
                ++*head;
            }
            continue;
        }

        if (chr == '#' || chr == ';' || (chr == '/' && chr_next == '/')) {
            ++*head;
            if(chr == '/')
                ++*head;
            while(*head < text->len) {
                if(text->ptr[*head] == '\n')
                    break;
                ++*head;
            }
            continue;
        }
        if (chr == '/' && chr_next == '*') {
            *head += 2;
            while(*head < text->len) {
                if(*head+2 < text->len && text->ptr[*head] == '*' && text->ptr[*head+1] == '/') {
                    *head+=2;
                    break;
                }
                ++*head;
            }
            continue;
        }

        if(chr == ' ' || chr == '\r' || chr == '\f' || chr == '\t' || chr == '\n') {
            ++*head;
            continue;
        }
        break;
    }
    return *head - origin_head;
}

int parse_string(string* text, int* head, string* str) {
    str->len = 0;
    str->ptr = nullptr;
    int origin_head = *head;

    char chr = text->ptr[*head];
    if(chr != '"') {
        return 0;
    }
    ++*head;
    str->ptr = &text->ptr[*head];
    while(*head < text->len) {
        // TODO: Handle escaped characters
        if(text->ptr[*head] == '"')
            break;
        ++*head;
        str->len++;
    }

    // IMPORTANT: Do not remove this allocation, code when processing includes relies
    //   on the returned memory not being invalidated.
    // IMPORTANT: Code also relies on it being null terminated at its proper length.
    char* buf = malloc(str->len + 1);
    memcpy(str->ptr, buf, str->len);
    buf[str->len] = '\0';
    str->ptr = buf;
    return *head - origin_head;
}

int skip_line(string* text, int* head) {
    int origin_head = *head;

    while(*head < text->len) {
        if(text->ptr[*head] == '\n')
            break;
        ++*head;
    }

    return *head - origin_head;
}
int get_regnr(string name) {
    if(name.len != 2)
        return -1;
    if(name.ptr[0] != 'r')
        return -1;
    if(name.ptr[1] < 'a' || name.ptr[1] > 'h')
        return -1;
    return name.ptr[1] - 'a';
}
Location count_lines(string* text, int head, const char* file) {
    Assert(head < text->len);
    Location loc = {file, 1, 1};
    int index = 0;
    while(index < head) {
        if (text->ptr[index] == '\n') {
            loc.line++;
            loc.column = 0;
        }

        index++;
        loc.column++;
    }
    return loc;
}