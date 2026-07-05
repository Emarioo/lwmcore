
section .text 0x0
#define UART_BASE 0x2000
main:
    li r0, 2
    li r1, 5
    add r0, r0, r1

    li r3, '0'
    add r0, r0, r3
    call putchar
    
    li r0, '\n'
    call putchar

    wfi

putchar:
    stb r0, [#UART_BASE] 
    ret
