/*
    TEST_CONFIG = all
*/

#include "tests/test_pre.s"

section .text 0

main:
    #TEST_PRE

    # Byte

    li r1, 0x90
    stb r1, [value8]

    li r0, 0
    ldb r0, [value8]
    #TEST(0x90)

    li r0, 0
    ldbs r0, [value8]
    #TEST(-112)

    # Halfword

    li r1, 0x8234
    sth r1, [value16]

    li r0, 0
    ldh r0, [value16]
    #TEST(0x8234)

    li r0, 0
    ldhs r0, [value16]
    #TEST(-0x7dcc)


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