/*
    TEST_CONFIG = all
*/

#include "tests/test_pre.s"

section .text 0

main:
    #TEST_PRE

    call test_gprs
    call test_context_ptr
    call test_stack_ptr

    #TEST_POST
    hlt

test_gprs:
    push lr

    lea r0, [context]

    li r3, 11
    li r4, 22
    li r9, 33
    li r13, 44
    li r14, 55

    # @TODO Test high registers on 32 and 64-bit.

    save r0

    li r3, 0
    li r4, 0
    li r9, 0
    li r13, 0
    li r14, 0

    restore r0

    push r14 // save link register

    mov r0, r3
    #TEST(11)

    mov r0, r4
    #TEST(22)

    mov r0, r9
    #TEST(33)

    mov r0, r13
    #TEST(44)

    pop r14

    mov r0, r14
    #TEST(55)

    pop lr
    ret

test_context_ptr:
    push lr


    li r0, 0x7F
    stb r0, [context]

    lea r0, [context]
    save r0

    ldh r0, [context]
    #TEST(0x7F)

    li r0, 0x40
    stb r0, [context]

    lea r0, [context]
    restore r0

    #TEST(0x40)

    pop lr
    ret

test_stack_ptr:
    push lr

    li r0, 0x7008
    mtcr CRESP, r0

    li r1, 1

    lea r0, [context]
    save r0

    # In the fure 'cpufeat' will provide the CPU bitwidth but for now
    # we can calculate it using this. We must calculate it to know where
    # SP is located in context.

    ldh r2, [context + 2]
    jz r2, .check32
    lea r4, [context + 30]
    jmp .end

.check32:
    ldh r2, [context + 4]
    jz r2, .check64
    lea r4, [context + 60]
    jmp .end

.check64:
    lea r4, [context + 120]
.end:

    ldh r0, [r4]
    #TEST(0x7008)

    li r0, 0x4100
    sth r0, [r4]

    li r0, 0
    mtcr CRESP, r0

    lea r0, [context]
    restore r0

    mfcr r0, CRESP
    #TEST(0x4100)

    pop lr
    ret

align 0x10
context:
    byte[256] // 256=32*8 for just general purpose registers

#include "tests/test_post.s"
