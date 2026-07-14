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

    # @TODO Consider testing 0x8234 on 16-bit cpu when sign extending halfword load.
    #    It should return correct result while on 32/64 it should be incorrect.


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