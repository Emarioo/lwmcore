
#define UART_BASE 0xF004
#define LOG_BASE  0xF008

#define PLATFORM_CORE_CONTROL 0xEFE0
#define PLATFORM_CORE_CPUID   0xEFE2
#define PLATFORM_CORE_ENTRY   0xEFE8

#define PLATFORM_CORE_CONTROL_BOOT 0x1
#define PLATFORM_CORE_CONTROL_RESET 0x2


section .boot 0x0
    jmp main

section .data 0x4

init_msg:
    byte[] "core0\n\0"
    
core_msg:
    byte[] "core1\n\0"

section .text 0x140

main:
    li sp, 0x1000

    lea r0, [init_msg]
    call putstring

    li r0, 1
    stb r0, [#PLATFORM_CORE_CPUID]
    lea r0, [core_entry]
    sth r0, [#PLATFORM_CORE_ENTRY]
    li r0, #PLATFORM_CORE_CONTROL_BOOT
    stb r0, [#PLATFORM_CORE_CONTROL]

inf:
    slow
    li r0, 'B'
    call putchar
    jmp inf


    hlt

core_entry:
    li sp, 0x1100

    lea r0, [core_msg]
    call putstring

    li r0, 0
    stb r0, [#PLATFORM_CORE_CPUID]
    li r0, #PLATFORM_CORE_CONTROL_RESET
    stb r0, [#PLATFORM_CORE_CONTROL]

    lea r0, [core_msg]
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

