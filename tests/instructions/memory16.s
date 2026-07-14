/*
    TEST_CONFIG = all
*/

#include "tests/test_pre.s"

section .text 0

main:
    #TEST_PRE

    # Byte

    li r0, 0x5A
    stb r0, [value8]

    li r0, 0
    ldb r0, [value8]
    #TEST(0x5A)

    # Halfword

    li r0, 0x1234
    sth r0, [value16]

    li r0, 0
    ldh r0, [value16]
    #TEST(0x1234)


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