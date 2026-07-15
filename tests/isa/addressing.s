/*
    TEST_CONFIG = all

    TODO:
      - Add explicit ABS32/ABS64 tests.
      - Add assembler tests forcing PC_DISP8/16/32 selection.
      - Add assembler tests forcing REG_DISP8/16/32 selection.
*/

#include "tests/test_pre.s"

section ".text" 0x0

main:
    #TEST_PRE


    #
    # LEA same label twice
    # Result must be identical.
    #

    lea r0, [value_a]
    lea r1, [value_a]

    sub r0, r0, r1
    #TEST(0)


    #
    # LEA two different labels
    # They should be 2 bytes apart (two halfwords).
    #

    lea r0, [value_a]
    lea r1, [value_b]

    sub r0, r1, r0
    #TEST(2)


    #
    # PC-relative load
    #

    ldh r0, [value_a]
    #TEST(0x1234)


    #
    # ABS16 load
    #

    ldh r0, [0x200]
    #TEST(0x1234)


    #
    # REG1 displacement load
    #

    lea r1, [value_a]

    ldh r0, [r1]
    #TEST(0x1234)


    #
    # REG1 displacement with offset
    #

    lea r1, [values]

    ldh r0, [r1 + 2]
    #TEST(0x5678)


    #
    # REG2 displacement load
    #

    lea r1, [values]

    li r2, 2

    ldh r0, [r1 + r2]
    #TEST(0x5678)


    #
    # Store with one addressing mode.
    # Load using another addressing mode.
    #

    li r0, 0xABCD

    lea r1, [store_value]

    sth r0, [r1]


    ldh r0, [store_value]
    #TEST(0xABCD)


    # TODO:
    # Add PC-relative displacement range tests:
    # PC_DISP8
    # PC_DISP16
    # PC_DISP32


    # TODO:
    # Add ABS32 and ABS64 tests using paging.


    #TEST_POST

    hlt


section ".data" 0x200

#
# Also tests ABS16:
# address is exactly 0x200
#

abs16_value:
value_a:
    short 0x1234

value_b:
    short 0x5678


values:
    short 0x1111
    short 0x5678


store_value:
    short 0


#include "tests/test_post.s"