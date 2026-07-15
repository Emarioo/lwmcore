
#define UART_BASE 0xF004
#define LOG_BASE  0xF008



/*
    void putchar(char chr);
      r0 = chr
*/
putchar:
    push r1
    
    stb r0, [#UART_BASE] 

    ldh r1, [log_index]
    stb r0, [#LOG_BASE + r1] 
    lea r1, [r1 + 1]
    sth r1, [log_index]

    pop r1
    ret

align 4
log_index:
    long


/*
    void putstring(char* str);
      r0 = str
*/
putstring:
    push lr
    push r1
    mov r1, r0

  putstring_loop:
    ldb r0, [r1]
    jz r0, putstring_end
    call putchar
    lea r1, [r1 + 1]
    jmp putstring_loop
  putstring_end:

    pop r1
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
