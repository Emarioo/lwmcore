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

typedef struct {
    int len;
    int max;
    char* ptr;
} Bin;

#define error(f, ...) printf("\e[31mERROR:\e[0m " f, ##__VA_ARGS__)
#define warning(f, ...) printf("\e[33mERROR:\e[0m " f, ##__VA_ARGS__)