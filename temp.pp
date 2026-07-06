

section .data 0x80

ex_msg:
    byte[] "EXCEPTION\n\0"
dbg_msg:
    byte[] "DEBUG\n\0"
done_msg:
    byte[] "DONE\n\0"

section .text 0

main:
    li sp, 0x100

    call init_vector

    dbg
    
    lea r0, [done_msg]
    call putstring

    hlt

putchar:
    stb r0, [0xF004] 

    stb r0, [r12 + 0xF008] 
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

    lea r0, [ex_handler_dbg]
    sth r0, [vector + 32]

    ret

section .eh 0xC0
ex_handler:
    lea r0, [ex_msg]
    call putstring
    hlt

ex_handler_dbg:
    lea r0, [dbg_msg]
    call putstring

    // Skip DBG instruction
    mfcr r0, CREPC
    li r1, 1
    add r0, r0, r1
    mtcr CREPC, r0

    vret


section .vector 0x100
vector:
    long[12]


