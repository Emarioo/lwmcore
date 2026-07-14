
#include "tests/test_pre.s"

section .text 0x0

main:
    #TEST_PRE

    li r0, 65
    li r1, 289
    add r0, r0, r1
    #TEST(354)

    li r0, 65
    li r1, 289
    sub r0, r0, r1
    #TEST(-224)

    #TEST_POST
    hlt

#include "tests/test_post.s"
