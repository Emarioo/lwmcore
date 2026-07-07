/*
    Inter-process Interrupts (IPI)
*/

#define UART_BASE 0xF004
#define LOG_BASE  0xF008

#define PLATFORM_CORE_CONTROL 0xEFE0
#define PLATFORM_CORE_CPUID   0xEFE2
#define PLATFORM_CORE_ENTRY   0xEFE8

#define PLATFORM_CORE_CONTROL_BOOT 0x1
#define PLATFORM_CORE_CONTROL_RESET 0x2

#define IC_CONTROL       0xE000
#define IC_IPI_CPUID     0xE004
#define IC_IPI_VECTOR    0xE006

#define IC_CONTROL_IPI_SEND_MASK  0x1

section .boot 0x0
    jmp main

section .data 0x4

init_msg:
    byte[] "core0\n\0"
    
core_msg:
    byte[] "core1\n\0"
    
ipi_msg:
    byte[] "ipi\n\0"

vector:
    long[12] #repeat 11 "ex_handler, " ex_handler


section .text 0x140

main:
    li sp, 0x1000

    
    lea r0, [vector]
    mtcr CRVB, r0

    lea r0, [ex_handler_ipi]
    sth r0, [vector + 128 + 0]

    eint

    lea r0, [init_msg]
    call putstring

    li r0, 1
    stb r0, [#PLATFORM_CORE_CPUID]
    lea r0, [core_entry]
    sth r0, [#PLATFORM_CORE_ENTRY]
    li r0, #PLATFORM_CORE_CONTROL_BOOT
    stb r0, [#PLATFORM_CORE_CONTROL]

inf:
    jmp inf

    hlt

core_entry:
    li sp, 0x1100

    lea r0, [core_msg]
    call putstring

    li r0, 0
    sth r0, [#IC_IPI_CPUID]
    li r0, 32
    sth r0, [#IC_IPI_VECTOR]
    li r0, #IC_CONTROL_IPI_SEND_MASK
    stb r0, [#IC_CONTROL]

inf:
    jmp inf

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


saved_ctx:
    short[4]

#define SAVE
    sth lr, [saved_ctx]
    sth r0, [saved_ctx+2]
    sth r1, [saved_ctx+4]
    sth r2, [saved_ctx+6]
#endmacro

#define RESTORE
    ldh lr, [saved_ctx]
    ldh r0, [saved_ctx+2]
    ldh r1, [saved_ctx+4]
    ldh r2, [saved_ctx+6]
#endmacro

ex_handler_ipi:
    mfcr sp, CRESP
    #SAVE

    lea r0, [ipi_msg]
    call putstring

    hlt

    #RESTORE
    vret

ex_handler:
    wfi


