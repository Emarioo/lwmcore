/*
    TEST_CONFIG = lwm32

    @TODO Test for huge pages
*/

#include "tests/test_pre.s"


#define CRSTATUS_USER        0x2
#define CRSTATUS_PAGING      0x4
#define CRSTATUS_INTERRUPT   0x8

#define PAGE_BIT_PRESENT     0x1
#define PAGE_BIT_USER        0x2
#define PAGE_BIT_READ        0x4
#define PAGE_BIT_WRITE       0x8
#define PAGE_BIT_EXECUTE     0x10

section .data 0x4000
short 0x99
short 0x0

section .text 0

main:
    #TEST_PRE

    /*
        Preparations
    */

    lea r0, [pageTable]
    li r1, 0x1F
    or r0, r0, r1
    stl r0, [rootTable]
    lea r0, [rootTable]
    mtcr CRPT, r0

    #include "tests/isa/_paging.s"

    #TEST_POST
    hlt


// r0 = 1024-index into page table
// r1 = Flags
// r2 = Physical Address bits 12:31 (low 12 bits should be aligned to page and are not needed)
map_page:
    li r4, 12
    shl r2, r2, r4
    or r2, r2, r1

    li r4, 4
    umul r0, r0, r4
    stl r2, [pageTable + r0]
    ret

#include "tests/test_post.s"

section .page_tables 0x8000
rootTable:
    long[1024]
pageTable:
    long[1024]


