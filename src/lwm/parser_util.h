#pragma once

#include "lwm/util.h"

#include <stdint.h>
#include <setjmp.h>



typedef struct {
    const char* file;
    int line;
    int column;
} Location;


typedef struct {
    int head_start;
    int head_end;
    string file;
    int line;
} LocationMapping;

#define MAX_LOCATION_MAPPING 100

typedef struct {
    const char* path;
    const char* text;
    int         text_len;

    int loc_map_len;
    LocationMapping loc_map[MAX_LOCATION_MAPPING]; // TODO: Increase limit

    jmp_buf     jumpBuffer;
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
    const char* text;
    int         text_len;
} ParserSaveState;

static inline ParserSaveState parse_save(ParserContext* context, const char* newText, int newLength) {
    ParserSaveState state = {
        .text = context->text,
        .text_len = context->text_len,
    };
    context->text = newText;
    context->text_len = newLength;
    return state;
}
static inline void parse_restore(ParserContext* context, const ParserSaveState state) {
    context->text = state.text;
    context->text_len = state.text_len;
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



Location count_lines(ParserContext* context, int inout_head);


bool preprocess_text(const string text, string* out_text);
