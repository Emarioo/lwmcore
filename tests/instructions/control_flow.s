/*
    TEST_CONFIG = all

    TODO:
      - Force CALL DISP8/DISP16/DISP32 encodings.
      - Force JMP DISP8/DISP16/DISP32 encodings.
      - Force conditional jump DISP8/DISP16/DISP32 encodings.
*/

#include "tests/test_pre.s"

section .text 0

main:
    #TEST_PRE

    #
    # CALL label
    #

    li r0, 0
    call func_call
    #TEST(5)


    #
    # CALL register
    #

    li r0, 0
    lea r1, [func_call]
    call r1
    #TEST(5)


    #
    # Nested CALL
    #

    li r0, 0
    call func_outer
    #TEST(9)


    #
    # JMP label
    #

    li r0, 1
    jmp jmp_label_fail

    li r0, 0          # Must never execute

jmp_label_fail:
    #TEST(1)


    #
    # JMP register
    #

    li r0, 2
    lea r1, [jmp_reg_target]
    jmp r1

    li r0, 0          # Must never execute

jmp_reg_target:
    #TEST(2)


    #
    # JZ taken
    #

    li r0, 0
    jz r0, jz_taken

    li r0, 1          # Must never execute

jz_taken:
    #TEST(0)


    #
    # JZ not taken
    #

    li r0, 7
    jz r0, jz_not_taken

    #TEST(7)

jz_not_taken:


    #
    # JNZ taken
    #

    li r0, 8
    jnz r0, jnz_taken

    li r0, 0          # Must never execute

jnz_taken:
    #TEST(8)


    #
    # JNZ not taken
    #

    li r0, 0
    jnz r0, jnz_not_taken

    #TEST(0)

jnz_not_taken:


    #
    # JEQ
    #

    li r0, 5
    li r1, 5

    jeq r0, r1, jeq_taken

    li r0, 0

jeq_taken:
    #TEST(5)


    #
    # JNE
    #

    li r0, 5
    li r1, 6

    jne r0, r1, jne_taken

    li r0, 0

jne_taken:
    #TEST(5)


    #
    # Signed comparisons
    #

    lis r0, -1
    li  r1, 1

    jlt r0, r1, jlt_taken
    li r0, 0

jlt_taken:
    #TEST(-1)


    lis r0, -1
    lis r1, -1

    jle r0, r1, jle_taken
    li r0, 0

jle_taken:
    #TEST(-1)


    li r0, 5
    lis r1, -1

    jgt r0, r1, jgt_taken
    li r0, 0

jgt_taken:
    #TEST(5)


    li r0, 5
    li r1, 5

    jge r0, r1, jge_taken
    li r0, 0

jge_taken:
    #TEST(5)


    #
    # Unsigned comparisons
    #

    lis r0, -1
    li  r1, 1

    ja r0, r1, ja_taken
    li r0, 0

ja_taken:
    #TEST(-1)


    lis r0, -1
    lis r1, -1

    jae r0, r1, jae_taken
    li r0, 0

jae_taken:
    #TEST(-1)


    li r0, 1
    lis r1, -1

    jb r0, r1, jb_taken
    li r0, 0

jb_taken:
    #TEST(1)


    li r0, 1
    li r1, 1

    jbe r0, r1, jbe_taken
    li r0, 0

jbe_taken:
    #TEST(1)


    #TEST_POST
    hlt


#
# Functions
#

func_call:
    li r0, 5
    ret


func_outer:
    push lr
    call func_inner
    pop lr

    li r0, 9
    ret


func_inner:
    li r0, 4
    ret


#include "tests/test_post.s"
