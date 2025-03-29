#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define nullptr ((void*)0)

typedef struct {
    int len;
    char* ptr;
} string;

bool equal(string text, const char* str);

typedef struct {
    int len;
    int max;
    char* ptr;
} Bin;

#define error_src(LOC,FORMAT, ...) printf("\e[31m%s:%d:%d:\e[0m " FORMAT, (LOC).file, (LOC).line, (LOC).column, ##__VA_ARGS__)
#define error(FORMAT, ...) printf("\e[31mERROR:\e[0m " FORMAT, ##__VA_ARGS__)
#define warning_src(LOC,FORMAT, ...) printf("\e[33m%s:%d:%d:\e[0m " FORMAT, (LOC).file, (LOC).line, (LOC).column, ##__VA_ARGS__)
#define warning(FORMAT, ...) printf("\e[33mWARNING:\e[0m " FORMAT, ##__VA_ARGS__)

#define log_src(LOC,FORMAT, ...) printf("\e[97m%s:%d:%d:\e[0m " FORMAT, (LOC).file, (LOC).line, (LOC).column, ##__VA_ARGS__)

#define Assert(EXPR) ((EXPR) ? true : (fprintf(stderr,"[Assert] %s (%s:%u)\n",#EXPR,__FILE__,__LINE__), *((char*)0) = 0))