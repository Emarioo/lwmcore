
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




SourceLocation get_location(ParserContext* context, int inout_head) {
    SourceSpan* foundSpan = nullptr;
    SourceSpan* foundRealSpan = nullptr;
    for(int i=0;i<context->spans_len;i++) {
        SourceSpan* span = &context->spans[i];
        if(inout_head >= span->dst_start && inout_head < span->dst_end) {
            foundSpan = span;
            while (span->file[0] == '<') {
                i--;
                Assert(i>=0);
                span = &context->spans[i];
            }
            foundRealSpan = span;
            break;
        }
    }
    Assert(foundSpan);

    char* buffer;
    size_t buffer_size;

    bool found = readFile(foundRealSpan->file, (void**)&buffer, &buffer_size);
    Assert(found);

    SourceLocation loc = { foundRealSpan->file, 1, 1, .dst_index = inout_head };
    int targetIndex;
    if (foundRealSpan != foundSpan) {
        targetIndex = foundRealSpan->src_end;
    } else {
        targetIndex = foundSpan->src_start + inout_head - foundSpan->dst_start;
    }
    
    int index = 0;
    while(index < targetIndex) {
        if (buffer[index] == '\n') {
            loc.line++;
            loc.column = 0;
        }

        index++;
        loc.column++;
    }
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


#define ERROR_SRC_RET(HEAD, FORMAT, ...) SourceLocation loc = get_location(context, HEAD); error_src(loc, FORMAT, ##__VA_ARGS__); longjmp(context->jumpBuffer, 1)



void push_text(ParserContext* context, int start, int end) {
    const int size = end - start;
    Assert(context->outputBuffer_len + size < context->outputBuffer_max);
    
    SourceSpan* lastSpan = context->spans_len > 0 ? &context->spans[context->spans_len-1] : NULL;
    if (lastSpan && lastSpan->file == context->path && lastSpan->src_end == start) {
        lastSpan->dst_end = context->outputBuffer_len + size;
        lastSpan->src_end = end;
    } else {
        Assert(context->spans_len+1 <= context->spans_max);
        context->spans_len++;
        SourceSpan* newSpan = &context->spans[context->spans_len-1];
        memset(newSpan, 0, sizeof(*newSpan));
        newSpan->file = context->path;
        newSpan->src_start = start;
        newSpan->src_end = end;
        newSpan->dst_start = context->outputBuffer_len;
        newSpan->dst_end = context->outputBuffer_len + size;
    }
    
    memcpy(context->outputBuffer + context->outputBuffer_len, context->text + start, size);
    context->outputBuffer_len += size;
    context->outputBuffer[context->outputBuffer_len] = '\0';
}

void push_raw_text(ParserContext* context, const char* file, const char* text, int size) {
    Assert(context->outputBuffer_len + size < context->outputBuffer_max);

    SourceSpan* lastSpan = context->spans_len > 0 ? &context->spans[context->spans_len-1] : NULL;

    if (lastSpan && lastSpan->file == file) {
        lastSpan->dst_end = context->outputBuffer_len + size;
    } else {
        Assert(context->spans_len+1 <= context->spans_max);
        context->spans_len++;
        SourceSpan* newSpan = &context->spans[context->spans_len-1];
        memset(newSpan, 0, sizeof(*newSpan));
        newSpan->file = file;
        newSpan->src_start = lastSpan->src_end;
        newSpan->src_end = lastSpan->src_end + 1;
        newSpan->dst_start = context->outputBuffer_len;
        newSpan->dst_end = context->outputBuffer_len + size;
    }
    
    memcpy(context->outputBuffer + context->outputBuffer_len, text, size);
    context->outputBuffer_len += size;
    context->outputBuffer[context->outputBuffer_len] = '\0';
}

bool preprocess_text(ParserContext* context, const string in_text, string* out_text, const char* sourcePath) {

    MacroDef* macros = calloc(100, sizeof(MacroDef));
    int macros_len = 0;

    
    ParserContext _context = {0};
    if (!context) {
        context = &_context;
    }
    
    context->path = sourcePath;
    context->text = in_text.ptr;
    context->text_len = in_text.len;

    context->outputBuffer_max = 0x10000 - 1;
    context->outputBuffer_len = 0;
    context->outputBuffer = malloc(context->outputBuffer_max + 1);

    int jmpResult = setjmp(context->jumpBuffer);
    if (jmpResult != 0) {
        // Error messages already printed.
        return false;
    }

    int head = 0;
    int parsedChars;

    context->spans_len = 0;
    context->spans_max = 200;
    context->spans = malloc(sizeof(SourceSpan) * context->spans_max);


    int              includeStack_len = 0;
    int              includeStack_max = 50;
    ParserSaveState* includeStack = malloc(sizeof(ParserSaveState) * includeStack_max);

    
    while (true) {
        
        parsedChars = parse_space(context, &head);
        push_text(context, head - parsedChars, head);
        
        if (parse_eof(context, head)) {
            if (includeStack_len > 0) {
                ParserSaveState* lastInclude = &includeStack[includeStack_len-1];
                parse_restore(context, *lastInclude, &head);
                includeStack_len--;
                continue;
            }
            break;
        }

        int headBeforeDirective = head;

        char chr = get_char(context, head);
        if (chr != '#') {
            head++;
            push_text(context, head - 1, head);
            continue;
        }
        head++;

        
        string macroName;
        int parsedChars = parse_name(context, &head, &macroName);
        if (!parsedChars) {
            ERROR_SRC_RET(head, "Expected a directive.\n");
        }

        if (equal(macroName, "include")) {
            parse_space(context, &head);

            string includePath;
            int parsedChars = parse_string(context, &head, &includePath);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a path in quotes.\n");
            }
            
            char*  fileBuffer;
            size_t fileBuffer_len;
            int res = readFile(includePath.ptr, (void**)&fileBuffer, &fileBuffer_len);
            if (!res) {
                ERROR_SRC_RET(head, "Could not open '%s'.\n", includePath.ptr);
            }

            includeStack_len++;
            ParserSaveState* lastInclude = &includeStack[includeStack_len-1];
            *lastInclude = parse_save(context, includePath.ptr, fileBuffer, fileBuffer_len, &head);

            continue;

        } else if (equal(macroName, "define")) {
            
            parsedChars = parse_space(context, &head);

            int parsedChars = parse_name(context, &head, &macroName);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a macro name.\n");
            }

            // printf("macro %s\n", macroName.ptr);

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
                parse_space_not_newline(context, &head);
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
            parsedChars = parse_space(context, &head);

            macro->body_len = body_end - body_start;
            macro->body = malloc(macro->body_len+1);
            memcpy(macro->body, context->text + body_start, macro->body_len);
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
                push_raw_text(context, "<repeat>", content.ptr, content.len);
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
                    argStart = head;
                    argEnd = head;
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

        ParserSaveState prevState = parse_save(context, "<macro_body>", body, body_len, &body_head);

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

                    int length = arg->end - arg->start;
                    push_raw_text(context, context->path, prevState.text + arg->start, length);
                } else {
                    // @TODO Error, missing arguments.
                }
                
            } else {
                push_raw_text(context, context->path, body + body_head, 1);
                body_head++;
            }
        }

        parse_restore(context, prevState, &body_head);

        #undef head


    }

    out_text->ptr = context->outputBuffer;
    out_text->len = context->outputBuffer_len;
    return true;
}
