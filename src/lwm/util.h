#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <setjmp.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define ARRAY_LENGTH(ARR) (sizeof(ARR)/sizeof(*ARR))

typedef struct {
    int len;
    char* ptr;
} string;

bool equal(string text, const char* str);
bool endswith(string text, const char* str);
int find_string(string text, const char* str);

bool create_folder(string path);
// file or folder
// bool remove_file(string path);

void sleep_us(uint64_t microseconds);

bool readFile(const char* path, void** buffer, size_t* size);
bool writeFile(const char* path, void* buffer, size_t size);

typedef struct {
    int len;
    int max;
    char* ptr;
} Bin;

#define error_src(LOC,FORMAT, ...) printf("\e[31m%s:%d:%d:@%d:\e[0m " FORMAT, (LOC).file, (LOC).line, (LOC).column, (LOC).dst_index, ##__VA_ARGS__)
#define error(FORMAT, ...) printf("\e[31mERROR:\e[0m " FORMAT, ##__VA_ARGS__)
#define warning_src(LOC,FORMAT, ...) printf("\e[33m%s:%d:%d:\e[0m " FORMAT, (LOC).file, (LOC).line, (LOC).column, ##__VA_ARGS__)
#define warning(FORMAT, ...) printf("\e[33mWARNING:\e[0m " FORMAT, ##__VA_ARGS__)

#define log_src(LOC,FORMAT, ...) printf("\e[97m%s:%d:%d:\e[0m " FORMAT, (LOC).file, (LOC).line, (LOC).column, ##__VA_ARGS__)

#define Assert(EXPR) ((EXPR) ? true : (fprintf(stderr,"[Assert] %s (%s:%u)\n",#EXPR,__FILE__,__LINE__), *((char*)0) = 0))


// NOTE: Variables below are defined in main.c

// fopen(..., "wb") are logged and not performed
extern bool DISABLE_FILE_WRITES;


static inline uint32_t lzcnt(uint32_t value) {
    return __builtin_clz(value);
    // #ifdef __x86_64__
    //     uint32_t result;
    //     __asm__( "lzcnt %1, %0" : "=r"(result), "r"(value)  )
    //     return result;
    // #else
    //     if (value == 0)
    //         return 32;
    //     int count = 0;
    //     while ((value & 0x80000000u) == 0) {
    //         count++;
    //         value <<= 1;
    //     }
    //     return count;
    // #endif
}

