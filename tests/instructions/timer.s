/*
    TEST_CONFIG = all

    @TODO What happens if you enable interrupts in exception handler?
       Not allowed? protection fault? it is allowed, timer interrupt jumps to it's handler and then
       back to exception handler? can cause bunch of confusion.

       Maybe we should store the insideFault, insideDoubleFault bits in CRSTATUS register?
       Writes to those bits are ignored maybe?
*/


#include "tests/test_pre.s"

section .text 0

main:
    #TEST_PRE

    /*
        Preparations
            Set exception handlers
    */
    call setup_vector_base


    /*
        Test 0
            call rdtick twice, the second tick value should be larger than the previous.
            spin for a bit to make sure it incremented at all.
    */
    rdtick r4

    li r1, 1
    li r2, 20
.spin0:
    sub r2, r2, r1
    jnz r2, .spin0

    rdtick r5

    sub r0, r5, r4
    #TEST_NE(0)


    /*
        Test 1
            enable interrupts
            schedule timer
            spin loop until timer_hit is set
            if it took longer than some amount of time then break loop
            and compare timer_hit with 1. it should be 0 since it wasn't hit.
    */
    li r0, 0
    sth r0, [hit_timer]
    eint
    li r0, 70
    advtimer r0

    li r1, 1
    li r2, 200
.spin1:
    sub r2, r2, r1
    ldh r0, [hit_timer]
    jnz r0, .end1
    jnz r2, .spin1
.end1:
    ldh r0, [hit_timer]
    #TEST(1)


    /*
        Test 2
            disable interrupts
            schedule timer
            spin for a while until we know timer should have been hit.
            break out of loop and check that timer_hit is zero.

            then enable interrupt and we should instantly get timer hit since timer interrupt was pending.
    */
    dint
    li r0, 0
    sth r0, [hit_timer]
    li r0, 40
    advtimer r0

    li r1, 1
    li r2, 100
.spin2:
    sub r2, r2, r1
    jnz r2, .spin2

    ldh r0, [hit_timer]
    #TEST(0)

    eint
    nop // Introduce a slight delay just to be safe
    nop
    ldh r0, [hit_timer]
    #TEST(1)


    /*
        Test 3
            enable interrupts
            schedule timer
            spin for a while until we hit timer twice
            set loop limit in case we never hit timer.
            once handler hits timer we should reschedule it again.
            this requires a different timer handler from the previous tests.
    */
    li r0, 9
    lea r1, [ex_handler_timer_with_reschedule]
    call set_handler
    
    li r0, 0
    sth r0, [hit_timer]
    eint
    li r0, 70
    advtimer r0

    li r1, 1
    li r2, 200
.spin3:
    sub r2, r2, r1
    ldh r0, [hit_timer]
    li r3, 3
    jeq r0, r3, .end3
    jnz r2, .spin3
.end3:
    ldh r0, [hit_timer]
    #TEST(3)


    /*
        Test 4
            enable interrupts
            schedule timer
            trigger illegal instruction handler
            spin for some time so that timer interrupt would have triggered.
            at end of illegal handler check that timer wasn't hit.
            set a variable in handler
            return from ill handler
            we should jump into timer interrupt
            check that variable is set in timer handler.
    */
    li r0, 9
    lea r1, [ex_handler_timer_ill]
    call set_handler

    eint
    li r0, 70
    advtimer r0
    li r0, 0
    sth r0, [hit_timer]

    li r0, 0
    sth r0, [done_illegal_spin]
    byte 0xFF

    nop // Timer interrupt should happen here when we return from illegal instruction handler

    #TEST_POST
    hlt


align 8
hit_timer:
    quad

align 8
done_illegal_spin:
    quad

ex_handler_ill:
    mfcr sp, CRESP
    push lr
    push r1
    push r2

    li r1, 1
    li r2, 200
.spin:
    sub r2, r2, r1
    jnz r2, .spin

    ldh r0, [hit_timer]
    #TEST(0)

    li r1, 1
    xadd r1, [done_illegal_spin]

    mfcr r0, CREPC
    lea r0, [r0 + 1]
    mtcr CREPC, r0
    pop r2
    pop r1
    pop lr
    vret

ex_handler_timer_with_reschedule:
    mfcr sp, CRESP
    push r1

    li r1, 1
    xadd r1, [hit_timer]

    li r1, 70
    advtimer r1

    pop r1
    vret

ex_handler_timer:
    mfcr sp, CRESP
    push r1

    li r1, 1
    xadd r1, [hit_timer]

    pop r1
    vret

ex_handler_timer_ill:
    mfcr sp, CRESP
    push lr
    push r1
    push r2

    ldh r0, [done_illegal_spin]
    #TEST(1)

    li r1, 1
    xadd r1, [hit_timer]

    pop r2
    pop r1
    pop lr
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
    #VECTOR(9, ex_handler_timer)

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
