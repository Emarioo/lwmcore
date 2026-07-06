
#include "lwm/parser_util.h"



int parse_name(ParserContext* context, int* inout_head, string* name) {
    name->len = 0;
    name->ptr = NULL;
    int origin_head = *inout_head;
    int head = *inout_head;

    const char* text = context->text;
    int text_len = context->text_len;

    char chr = text[head];
    if (!(((chr|32) >= 'a' && (chr|32) <= 'z') || chr == '_')) {
        return 0;
    }
    const char* start_ptr = &text[head];
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

int parse_non_space(ParserContext* context, int* inout_head, string* name) {
    name->len = 0;
    name->ptr = NULL;
    int origin_head = *inout_head;
    int head = *inout_head;

    const char* text = context->text;
    int text_len = context->text_len;

    char chr = text[head];
    if (is_space(chr)) {
        return 0;
    }
    const char* start_ptr = &text[head];
    while(head < text_len) {
        chr = text[head];
        if (!is_space(chr)) {
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

int parse_int(ParserContext* context, int* inout_head, uint64_t* value) {
    int origin_head = *inout_head;
    int head = *inout_head;
    
    const char* text = context->text;
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
                tempValue = 16 * tempValue + (chr|32) - 'a' + 10;
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

int parse_space_not_newline(ParserContext* context, int* inout_head) {

    int origin_head = *inout_head;
    int head = *inout_head;

    const char* text = context->text;
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

        if(chr == ' ' || chr == '\r' || chr == '\f' || chr == '\t') {
            head++;
            continue;
        }
        break;
    }
    *inout_head = head;
    return head - origin_head;
}

int parse_space(ParserContext* context, int* inout_head) {

    int origin_head = *inout_head;
    int head = *inout_head;

    const char* text = context->text;
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

int parse_string(ParserContext* context, int* inout_head, string* str) {
    const char* text = context->text;
    int text_len = context->text_len;

    str->len = 0;
    str->ptr = NULL;
    int origin_head = *inout_head;
    int head = *inout_head;

    char chr = text[head];
    if(chr != '"') {
        return 0;
    }
    ++head;
    str->ptr = (char*)&text[head];

    // IMPORTANT: Do not remove this allocation, code when processing includes relies
    //   on the returned memory not being invalidated.
    // IMPORTANT: Code also relies on it being null terminated at its proper length.
    int maxLength = 200;
    str->ptr = malloc(maxLength + 1);

    while(head < text_len) {
        if (text[head] == '\\') {
            if (text[head+1] == 'n') {
                str->ptr[str->len] = '\n';
                str->len++;
                head += 2;
                continue;
            } else if (text[head+1] == '0') {
                str->ptr[str->len] = '\0';
                str->len++;
                head += 2;
                continue;
            } else {
                // TODO: Handle escaped characters
                return 0;
            }
        }
        if(text[head] == '"') {
            ++head;
            break;
        }
        str->ptr[str->len] = text[head];
        str->len++;
        ++head;
    }

    str->ptr[str->len] = '\0';

    Assert(str->len <= maxLength);

    *inout_head = head;
    return head - origin_head;
}

int parse_char(ParserContext* context, int* inout_head, char* out_chr) {
    const char* text = context->text;
    int text_len = context->text_len;

    int origin_head = *inout_head;
    int head = *inout_head;

    char chr = text[head];
    if(chr != '\'') {
        return 0;
    }
    ++head;

    if (text[head] == '\\') {
        if (text[head+1] == 'n') {
            *out_chr = '\n';
            head += 2;
        } else if (text[head+1] == '0') {
            *out_chr = '\0';
            head += 2;
        } else {
            // TODO: Handle escaped characters
            return 0;
        }
    } else {
        *out_chr = text[head];
        head++;
    }
    
    if(text[head] != '\'') {
        return 0;
    }
    ++head;

    *inout_head = head;
    return head - origin_head;
}

int parse_line(ParserContext* context, int* inout_head) {
    int origin_head = *inout_head;
    int head = *inout_head;

    const char* text = context->text;
    int text_len = context->text_len;

    while(head < text_len) {
        if(text[head] == '\n')
            break;
        ++head;
    }
    *inout_head = head;
    return head - origin_head;
}




Location count_lines(ParserContext* context, int inout_head) {
    int head = inout_head;
    const char* text = context->text;

    Location loc = { context->path, 1, 1 };
    
    int index = 0;
    while(index < head) {
        if (text[index] == '\n') {
            loc.line++;
            loc.column = 0;
        }

        index++;
        loc.column++;
    }

    // @TODO Implement location mapping needed to accurate locations in error messages.
    //    Stripped comments and included files is not accounted for with the above.
    // LocationMapping* map = nullptr;
    // for(int i=0;i<context->loc_map_len;i++) {
    //     map = &context->loc_map[i];
    //     if(head >= map->head_start && head < map->head_end)
    //         break;
    // }
    // // Assert(map);
    // Location loc = {map->file.ptr, map->line, 1};
    // int index = map->head_start;
    // while(index < head) {
    //     if (text[index] == '\n') {
    //         loc.line++;
    //         loc.column = 0;
    //     }

    //     index++;
    //     loc.column++;
    // }
    return loc;
}


typedef struct {
    char name[32];
} MacroParameter;

typedef struct {
    char name[64];
    int            parameters_len;
    MacroParameter parameters[5];

    char* body;
    int   body_len;
} MacroDef;


#define ERROR_SRC_RET(HEAD, FORMAT, ...) Location loc = count_lines(context, HEAD); error_src(loc, FORMAT, ##__VA_ARGS__); longjmp(context->jumpBuffer, 1)


bool preprocess_text(const string in_text, string* out_text) {

    MacroDef* macros = calloc(100, sizeof(MacroDef));
    int macros_len = 0;


    const char* text = in_text.ptr;
    int         text_len = in_text.len;

    ParserContext _context = {0};
    ParserContext* context = &_context;

    context->text = text;
    context->text_len = text_len;

    int outputBuffer_max = 0x10000;
    int outputBuffer_len = 0;
    char* outputBuffer = malloc(outputBuffer_max);

    int jmpResult = setjmp(context->jumpBuffer);
    if (jmpResult != 0) {
        // Error messages already printed.
        return false;
    }

    #define APPEND_BYTES(END,SIZE) \
        for (int bi=0;bi<(SIZE);bi++) { \
            outputBuffer[outputBuffer_len] = context->text[(END) - (SIZE) + bi]; \
            outputBuffer_len++; \
        }

    int head = 0;
    int parsedChars;

    while (true) {

        parsedChars = parse_space(context, &head);
        APPEND_BYTES(head, parsedChars);

        if (parse_eof(context, head))
            break;

        char chr = get_char(context, head);
        if (chr != '#') {
            head++;
            APPEND_BYTES(head, 1);
            continue;
        }
        head++;
        
        string macroName;
        int parsedChars = parse_name(context, &head, &macroName);
        if (!parsedChars) {
            ERROR_SRC_RET(head, "Expected a directive.\n");
        }

        if (equal(macroName, "define")) {
            
            parse_space(context, &head);

            int parsedChars = parse_name(context, &head, &macroName);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a macro name.\n");
            }

            bool hasParams = false;

            char chr = get_char(context, head);
            if (chr == '(') {
                hasParams = true;
                head++;
            }

            MacroDef* macro = &macros[macros_len];
            macros_len++;
            strcpy(macro->name, macroName.ptr);
            macro->parameters_len = 0;
            macro->body_len = 0;
            
            if (hasParams) {
                while (true) {
                    parse_space(context, &head);
                    
                    char chr = get_char(context, head);
                    if (chr == ')') {
                        head++;
                        break;
                    }

                    string parameterName;
                    parsedChars = parse_name(context, &head, &parameterName);
                    if (parsedChars) {
                        MacroParameter* parameter = &macro->parameters[macro->parameters_len];
                        macro->parameters_len++;
                        strcpy(parameter->name, parameterName.ptr);
                        
                        parse_space(context, &head);
                    }

                    chr = get_char(context, head);
                    if (chr == ',') {
                        head++;
                        continue;
                    } else if (chr != ')') {
                        ERROR_SRC_RET(head, "Expected parenthesis or comma.\n");
                    }
                }
                parse_space(context, &head);
            }

            int body_start = head;
            int body_end = head;
            bool hadNonSpace = false;
            bool reachedNewLine = false;

            while (true) {

                if (parse_eof(context, head)) {
                    ERROR_SRC_RET(head, "Expected #endmacro, got end of file.\n");
                }

                char chr = get_char(context, head);
                if (chr != '#') {
                    head++;
                    if (chr == '\n') {
                        if (hadNonSpace && !reachedNewLine) {
                            // endmacro is not expected in this cases:
                            //   #define MACRO()  sometext
                            // But it is expected here:
                            //   #define MACRO()
                            //   sometext
                            //   #endmacro
                            break;
                        }
                        reachedNewLine = true;
                    }
                    if (!is_space(chr)) {
                        hadNonSpace = true;
                        body_end = head;
                    } else {
                        if (!hadNonSpace) {
                            body_start = head;
                        }
                    }
                    continue;
                }
                head++;

                string word;
                parse_name(context, &head, &word);

                if (equal(word, "endmacro")) {
                    break;
                }
            }
            
            // Remove extra whitespace
            parse_space(context, &head);

            macro->body_len = body_end - body_start;
            macro->body = malloc(macro->body_len+1);
            memcpy(macro->body, text + body_start, macro->body_len);
            macro->body[macro->body_len] = 0;
            continue;
        } else if (equal(macroName, "repeat")) {
            
            parse_space(context, &head);

            uint64_t count;
            parsedChars = parse_int(context, &head, &count);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected integer.\n");
            }
            
            parse_space(context, &head);

            string content;
            parsedChars = parse_string(context, &head, &content);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected string.\n");
            }
            
            for (int ri=0;ri<count;ri++) {
                for (int bi=0;bi<content.len;bi++) {
                    outputBuffer[outputBuffer_len] = content.ptr[bi];
                    outputBuffer_len++;
                }
            }

            continue;
        }

        MacroDef* foundMacro = NULL;
        for (int mi=0;mi<macros_len;mi++) {
            MacroDef* macro = &macros[mi];
            if (equal(macroName, macro->name)) {
                foundMacro = macro;
                break;
            }
        }

        if (!foundMacro) {
            ERROR_SRC_RET(head, "Not a defined macro.\n");
        }

        typedef struct {
            int start;
            int end;
        } Argument;

        Argument arguments[8] = {0};
        int      arguments_len = 0;

        bool hadParams = false;

        chr = get_char(context, head);
        if (chr == '(') {
            hadParams = true;
            head++;
        }

        
        if (hadParams) {
            int argStart = head;
            int argEnd = head;
            while (true) {

                if (parse_eof(context, head)) {
                    ERROR_SRC_RET(head, "Expected #endmacro, got end of file.\n");
                }

                parse_space(context, &head);
                if (argStart == argEnd) {
                    argStart = head;
                    argEnd = head;
                }
                
                char chr = get_char(context, head);
                if (chr == ')' || chr == ',') {
                    if (argStart != argEnd) {
                        Argument* arg = &arguments[arguments_len];
                        arguments_len++;

                        arg->start = argStart;
                        arg->end = argEnd;
                    }

                    head++;
                    if (chr == ')') {
                        break;
                    }
                    continue;
                }
                
                if (!is_space(chr)) {
                    head++;
                    argEnd = head;
                }
            }
        }

        char* body = foundMacro->body;
        int   body_len = foundMacro->body_len;
        int   body_head = 0;

        #define head INCOMPLETE // make sure we don't accidently use it

        ParserSaveState prevState = parse_save(context, body, body_len);

        while (body_head < body_len) {

            char chr = body[body_head];
            if (chr == '%') {
                body_head++;

                string argname;
                parsedChars = parse_name(context, &body_head, &argname);
                if (!parsedChars) {
                    ERROR_SRC_RET(body_head, "Expected argument name.\n");
                }
                
                char chr = body[body_head];
                if (chr != '%') {
                    ERROR_SRC_RET(body_head, "Expected %% after argument name.\n");
                }
                body_head++;

                MacroParameter* foundParam = NULL;
                int foundParamIndex = -1;
                for (int ai=0;ai<foundMacro->parameters_len;ai++) {
                    MacroParameter* param = &foundMacro->parameters[ai];
                    if (equal(argname, param->name)) {
                        foundParam = param;
                        foundParamIndex = ai;
                        break;
                    }
                }

                if (!foundParam) {
                    ERROR_SRC_RET(body_head, "%s is not a parameter of %s.\n", argname.ptr, foundMacro->name);
                }

                if (foundParamIndex < arguments_len) {
                    Argument* arg = &arguments[foundParamIndex];

                    // @TODO Check max outputbuffer size.
                    int length = arg->end - arg->start;
                    memcpy(outputBuffer + outputBuffer_len, text + arg->start, length);
                    outputBuffer_len += length;
                } else {
                    // @TODO Error, missing arguments.
                }


            } else {
                body_head++;
                outputBuffer[outputBuffer_len] = chr;
                outputBuffer_len++;
            }
        }

        parse_restore(context, prevState);

        #undef head


    }

    out_text->ptr = outputBuffer;
    out_text->len = outputBuffer_len;
    return true;
}
