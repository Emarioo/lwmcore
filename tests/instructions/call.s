
#include "tests/test_pre.s"

section .text 0x0

main:
    #TEST_PRE

    call hello

    #TEST_POST
    hlt

hello:
    push lr

    li r0, 5
    #TEST(5)

    pop lr
    ret

#include "tests/test_post.s"
