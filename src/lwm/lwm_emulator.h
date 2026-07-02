

#pragma once

#include <stdint.h>
#include <stdbool.h>


typedef enum {
    MODE_16,
    MODE_32,
    MODE_64,
} CoreMode;


typedef struct {

    CoreMode mode;

    uint64_t gprs[32];
    uint64_t pc;
    #define tp gprs[13]
    #define lr gprs[14]
    #define sp gprs[15]

    uint64_t crs[32];

    #define crstatus crs[0]
    #define crvb     crs[1]
    #define crpt     crs[2]
    #define crepc    crs[3]
    #define crcause  crs[4]
    #define crfault  crs[5]
    #define crcpuid  crs[6]

    uint64_t tickCounter;

} CoreState;


typedef struct {

    uint64_t core_entry;

} PlatformConfig;


#define CRSTATUS_USER        0x1
#define CRSTATUS_PAGING      0x2
#define CRSTATUS_INTERRUPT   0x4