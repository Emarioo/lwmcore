/*
    TEST_CONFIG = all
*/

#include "tests/test_pre.s"

section .text 0

main:
    #TEST_PRE

    call setup_vector_base

    li r8, 11
    byte[2] 0xFF, 0xFF

    li r0, 5
    li r1, 0
    li r8, 22
    lea r9, [after_div]
    udiv r0, r0, r1
after_div:

    li r8, 33
    dbg

    #TEST_POST
    hlt


ex_handler_ill:
    mfcr sp, CRESP
    mov r0, r8

    push lr
    #TEST(11)
    pop lr

    mfcr r0, CREPC
    lea r0, [r0 + 1]
    mtcr CREPC, r0
    vret

ex_handler_div:
    mfcr sp, CRESP
    mov r0, r8

    push lr
    #TEST(22)
    pop lr
    
    mtcr CREPC, r9
    vret

ex_handler_dbg:
    mfcr sp, CRESP
    mov r0, r8

    push lr
    #TEST(33)
    pop lr

    mfcr r0, CREPC
    lea r0, [r0 + 1]
    mtcr CREPC, r0
    vret

ex_handler:
    hlt


setup_vector_base:
    push lr
    push r0
    push r1

    li r1, 0
    cpufeat r0, r1
    li r1, 8
    jeq r0, r1, .setup64

    lea r0, [vectors32]
    mtcr CRVB, r0

    jmp .end
.setup64:
    
    lea r0, [vectors64]
    mtcr CRVB, r0
.end:

    #define VECTOR(INDEX, HANDLER)
        li r0, %INDEX%
        lea r1, [%HANDLER%]
        call set_handler
    #endmacro

    #VECTOR(1, ex_handler_ill)
    #VECTOR(7, ex_handler_div)
    #VECTOR(8, ex_handler_dbg)

    pop r1
    pop r0
    pop lr
    ret

/*
    r0 = vector index
    r1 = handler
*/
set_handler:
    push r2

    li r2, 4
    umul r2, r2, r0
    sth r1, [vectors32 + r2]

    li r2, 8
    umul r2, r2, r0
    sth r1, [vectors64 + r2]

    pop r2
    ret

align 0x10
vectors32:
    long[12] #repeat 11 "ex_handler," ex_handler

align 0x10
vectors64:
    quad[12] #repeat 11 "ex_handler," ex_handler


#include "tests/test_post.s"
