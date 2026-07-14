/*
    TEST_CONFIG = min_lwm32
*/

#include "tests/test_pre.s"

section .text 0

main:
    #TEST_PRE

    # Long

    li r0, 0x12345678
    stl r0, [value32]

    li r0, 0
    ldl r0, [value32]
    #TEST(0x12345678)


    #TEST_POST
    hlt

value8:
    byte 0

value16:
    short 0

value32:
    long 0

value64:
    quad 0

#include "tests/test_post.s"