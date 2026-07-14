

#define UART_BASE 0xF004
#define LOG_BASE 0xF008

section .data 0xa0

ex_msg:
    byte[] "EXCEPTION\n\0"
timer_msg:
    byte[] "TIMER\n\0"
done_msg:
    byte[] "DONE\n\0"

hit_timer:
    byte 0

counter:
    long 0

section .text 0


main:
    li sp, 0x100

    call init_vector

    rdtick r0, r1, r2, r3

    eint
    
    li r4, 70
    add r0, r0, r4
    mtcr CRTIMERCMP, r0

loop:
    ldl r6, [counter]
    li r5, 1
    add r6, r6, r5
    stl r6, [counter]

    ldb r5, [hit_timer]
    jz r5, loop

    lea r0, [done_msg]
    call putstring
    
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

section .eh 0xC0
ex_handler:
    mfcr sp, CRESP
    lea r0, [ex_msg]
    call putstring
    hlt

section .eh2 0xD0
ex_handler_timer:
    mfcr sp, CRESP
    mov r9, r0
    lea r0, [timer_msg]
    call putstring

    li r10, 1
    stb r10, [hit_timer]

    mov r0, r9
    vret


section .vector 0x100
vector:
    long[12]
