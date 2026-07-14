/*
    TEST_CONFIG = min_lwm64
*/

#include "tests/test_pre.s"

section .text 0

main:
    #TEST_PRE

    # @TODO Test that quad load causes illegal instruction on 16-bit CPU.
    # Quad
    li r0, 0x1234567891011121
    stq r0, [value64]

    li r0, 0
    ldq r0, [value64]
    #TEST(0x1234567891011121)

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