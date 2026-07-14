#pragma once

#include "lwm/util.h"

#include <stdint.h>
#include <setjmp.h>



typedef struct {
    const char* file;
    int line;
    int column;
    int dst_index;
} SourceLocation;


typedef struct {
    int src_start;
    int src_end;
    int dst_start;
    int dst_end;
    const char* file;
    // line and column info are computed as needed. Can be cached in the future if needed.
} SourceSpan;


typedef struct {
    const char* path;
    const char* text;
    int         text_len;

    int         spans_len;
    int         spans_max;
    SourceSpan* spans;

    int   outputBuffer_max;
    int   outputBuffer_len;
    char* outputBuffer;

    jmp_buf jumpBuffer;
} ParserContext;


#define is_space(chr) ( chr == ' ' || chr == '\r' || chr == '\f' || chr == '\t' || chr == '\n' )

int parse_space(ParserContext* context, int* inout_head);
int parse_space_not_newline(ParserContext* context, int* inout_head);
int parse_line(ParserContext* context, int* inout_head);
int parse_name(ParserContext* context, int* inout_head, string* name);
int parse_non_space(ParserContext* context, int* inout_head, string* name) ;
int parse_string(ParserContext* context, int* inout_head, string* str);
int parse_char(ParserContext* context, int* inout_head, char* out_chr);
int parse_int(ParserContext* context, int* inout_head, uint64_t* value);


typedef struct {
    const char* path;
    const char* text;
    int         text_len;
    int         prev_position;
} ParserSaveState;

static inline ParserSaveState parse_save(ParserContext* context, const char* path, const char* newText, int newLength, int* position) {
    ParserSaveState state = {
        .path = context->path,
        .text = context->text,
        .text_len = context->text_len,
        .prev_position = *position,
    };
    context->path = path;
    context->text = newText;
    context->text_len = newLength;
    *position = 0;
    return state;
}

static inline void parse_restore(ParserContext* context, const ParserSaveState state, int* position) {
    context->path = state.path;
    context->text = state.text;
    context->text_len = state.text_len;
    *position = state.prev_position;
}

static inline int parse_eof(ParserContext* context, int inout_head) {
    return inout_head >= context->text_len;
}

static inline bool next_is_newline(ParserContext* context, int inout_head) {
    if (inout_head >= context->text_len)
        return false;
    return context->text[inout_head] == '\r' || context->text[inout_head] == '\n';
}

static inline char get_char(ParserContext* context, int inout_head) {
    if (inout_head >= context->text_len)
        return 0;
    return context->text[inout_head];
}



SourceLocation get_location(ParserContext* context, int inout_head);


bool preprocess_text(ParserContext* context, const string text, string* out_text, const char* sourcePath);
