/*
    TEST_CONFIG = all
*/

#include "tests/test_pre.s"

section .text 0

main:
    #TEST_PRE

    // For now we test that reg bytes features returns non-zero.
    // In the future we handcraft various platform configs and test 
    // specific cpu features (floating point, exact register byte size, SIMD).

    li r1, 0 // CPUFEAT_REG_BYTES
    cpufeat r0, r1
    jz r0, .end
    li r0, 1
.end:
    #TEST(1)

    #TEST_POST
    hlt


#include "tests/test_post.s"
