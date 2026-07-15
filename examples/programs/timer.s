

#define UART_BASE 0xF004
#define LOG_BASE 0xF008

section .text 0

main:
    li sp, 0x1000

    call init_vector

    rdtick r0, r1, r2, r3

    eint

    /*
    Observed values:
        70 ticks  -> counter = 12
        140 ticks -> counter = 23
    */
    li r4, 70
    ; add r0, r0, r4
    ; mtcr CRTIMERCMP, r0
    advtimer r4

loop:
    ldh r6, [counter]
    li r5, 1
    add r6, r6, r5
    sth r6, [counter]

    ldb r5, [hit_timer]
    jz r5, loop

    lea r0, [done_msg]
        call putstring

    ldh r0, [counter]
        call putint

    li r0, '\n'
        call putchar
    
    hlt


putchar:
    stb r0, [#UART_BASE] 

    stb r0, [r12 + #LOG_BASE] 
    li r0, 1
    add r12, r12, r0
    ret
    
putstring:
    push lr
    li r2, 1
    mov r1, r0

  putstring_loop:
    ldb r0, [r1]
    jz r0, putstring_end
    call putchar
    add r1, r1, r2
    jmp putstring_loop
  putstring_end:

    pop lr
    ret


/*
    void putint(int num);
      r0 = num
*/
putint:
    push lr
    push r1
    push r2
    push r3

    mov r2, r0

    lea r3, [sp + -1]
    lea sp, [sp - 24]

    li r0, '\0'
    stb r0, [r3]

.loop:
    li r1, 10
    umod r0, r2, r1
    udiv r2, r2, r1

    li r1, '0'
    add r0, r0, r1
    lea r3, [r3 - 1]
    stb r0, [r3]
    
    jnz r2, .loop

    mov r0, r3
    call putstring

    lea sp, [sp + 24]

    pop r3
    pop r2
    pop r1
    pop lr
    ret



init_vector:
    lea r0, [vector]
    mtcr CRVB, r0

    lea r0, [ex_handler]
    li r2, 4
    li r1, 12 // we have 11 exceptions at the moment
    umul r1, r1, r2

  init_vector_loop:
    sth r0, [r1 + vector + -4]
    sub r1, r1, r2
    jnz r1, init_vector_loop

    lea r0, [ex_handler_timer]
    sth r0, [vector + 36]

    ret

ex_msg:
    byte[] "EXCEPTION\n\0"
timer_msg:
    byte[] "TIMER\n\0"
done_msg:
    byte[] "Counter: \0"

hit_timer:
    byte 0

align 4
counter:
    long 0

ex_handler:
    mfcr sp, CRESP
    lea r0, [ex_msg]
    call putstring
    hlt

ex_handler_timer:
    mfcr sp, CRESP
    mov r9, r0
    lea r0, [timer_msg]
    call putstring

    li r10, 1
    stb r10, [hit_timer]

    mov r0, r9
    vret


align 0x10
vector:
    long[12]
