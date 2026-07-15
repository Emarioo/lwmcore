/*
    TEST_CONFIG = min_lwm32
*/

#include "tests/test_pre.s"

section .text 0

main:
    #TEST_PRE

    # Long

    li r0, 0x82345678
    stl r0, [value32]

    li r0, 0
    ldl r0, [value32]
    #TEST(0x82345678)

    li r0, 0
    ldls r0, [value32]
    #TEST(-0x7dcba988)

    #TEST_POST
    hlt

value8:
    byte 0

align 2
value16:
    short 0

align 4
value32:
    long 0

align 8
value64:
    quad 0

#include "tests/test_post.s"