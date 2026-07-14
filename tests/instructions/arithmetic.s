#include "tests/test_pre.s"

section .text 0x0

main:
    #TEST_PRE

    // Addition
    li r0, 65
    li r1, 289
    add r0, r0, r1
    #TEST(354)

    // Subtraction
    li r0, 65
    li r1, 289
    sub r0, r0, r1
    #TEST(-224)

    // Unsigned multiply
    li r0, 17
    li r1, 13
    umul r0, r0, r1
    #TEST(221)

    // Unsigned divide
    li r0, 221
    li r1, 13
    udiv r0, r0, r1
    #TEST(17)

    // Unsigned remainder
    li r0, 223
    li r1, 13
    umod r0, r0, r1
    #TEST(2)

    // Signed multiply
    li r0, -7
    li r1, 9
    smul r0, r0, r1
    #TEST(-63)

    // Signed divide
    li r0, -63
    li r1, 9
    sdiv r0, r0, r1
    #TEST(-7)

    // Signed remainder
    li r0, -65
    li r1, 9
    smod r0, r0, r1
    #TEST(-2)

    // Shift left
    li r0, 1
    li r1, 8
    shl r0, r0, r1
    #TEST(256)

    // Logical shift right
    li r0, 256
    li r1, 8
    shr r0, r0, r1
    #TEST(1)

    // AND
    li r0, 0xAA
    li r1, 0xCC
    and r0, r0, r1
    #TEST(0x88)

    // OR
    li r0, 0xAA
    li r1, 0xCC
    or r0, r0, r1
    #TEST(0xEE)

    // XOR
    li r0, 0xAA
    li r1, 0xCC
    xor r0, r0, r1
    #TEST(0x66)

    // NOT
    li r0, 0x0100
    not r0, r0
    #TEST(0xFEFF)

    #TEST_POST
    hlt

#include "tests/test_post.s"