





section .data 0x50
msg:
    byte[] "Hello from defs\n\0"

section .text 0x0
    jmp main

main:
    li sp, 0x1000

    lea r0, [msg]
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



