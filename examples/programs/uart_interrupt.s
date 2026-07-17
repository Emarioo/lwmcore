

#define UART_BASE 0xF004
#define LOG_BASE 0xF008

section .data 0xa4

ex_msg:
    byte[] "EXCEPTION\n\0"
uart_msg:
    byte[] "UART: \0"

align 4
counter:
    long 0

section .text 0


main:
    li sp, 0x100

    call init_vector

    call uart_rx_interrupt_enable

    li r0, 1
    li r1, 33
    li r2, 0
    call ic_map_irq

    eint

    li r0, 'S'
    call uart_write8

    /* Polling
  has_data:
    slow
    call uart_rx_ready
    jz r0, has_data
    
    call uart_read8
    call putchar
    jmp has_data
    */

inf:
    slow
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



init_vector:
    lea r0, [vector]
    mtcr CRVB, r0

    lea r0, [ex_handler_uart_rx]
    sth r0, [vector + 128 + 4]

    ret

section ".data"
align 2
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

section ".text"

ex_handler_uart_rx:
    mfcr sp, CRESP
    #SAVE

    call uart_rx_ready
    jz r0, no_data
    lea r0, [uart_msg]
    call putstring

  has_data:
    call uart_rx_ready
    jz r0, no_data
    call uart_read8
    call putchar
    jmp has_data
  no_data:

    #RESTORE
    vret

section .eh 0xE0
ex_handler:
    // mfcr sp, CRESP
    // lea r0, [ex_msg]
    // call putstring
    wfi



section .vector 0x100
vector:
    long[12] #repeat 11 "0xE0, " 0xE0


section .dev 0x200


#define UART_STATUS   0xE120
#define UART_CONTROL  0xE124
#define UART_DATA     0xE128
// Read-only
#define UART_IRQ      0xE12C

// Status register
#define UART_TX_READY_MASK  0x2
#define UART_TX_READY_BIT   1
#define UART_RX_READY_MASK  0x4
#define UART_RX_READY_BIT   2

// Control register
#define UART_TX_INTERRUPT_ENABLE_MASK   0x2
#define UART_TX_INTERRUPT_DISABLE_MASK  0xFD
#define UART_RX_INTERRUPT_ENABLE_MASK   0x4
#define UART_RX_INTERRUPT_DISABLE_MASK  0xFB

uart_irq_number:
    ldh r0, [#UART_IRQ]
    ret

uart_tx_interrupt_enable:
    ldb r0, [#UART_CONTROL]
    li r1, #UART_TX_INTERRUPT_ENABLE_MASK
    or r0, r0, r1
    stb r0, [#UART_CONTROL]
    ret

uart_tx_interrupt_disable:
    ldb r0, [#UART_CONTROL]
    li r1, #UART_TX_INTERRUPT_DISABLE_MASK
    and r0, r0, r1
    stb r0, [#UART_CONTROL]
    ret
    
uart_rx_interrupt_enable:
    ldb r0, [#UART_CONTROL]
    li r1, #UART_RX_INTERRUPT_ENABLE_MASK
    or r0, r0, r1
    stb r0, [#UART_CONTROL]
    ret

uart_rx_interrupt_disable:
    ldb r0, [#UART_CONTROL]
    li r1, #UART_RX_INTERRUPT_DISABLE_MASK
    and r0, r0, r1
    stb r0, [#UART_CONTROL]
    ret

uart_write8:
  uart_write8_wait:
    ldb r1, [#UART_STATUS]
    li r2, #UART_TX_READY_MASK
    and r1, r1, r2
    jz r1, uart_write8_wait

    stb r0, [#UART_DATA]
    ret

uart_rx_ready:
    ldb r0, [#UART_STATUS]
    li r2, #UART_RX_READY_MASK
    and r0, r0, r2
    // shr r0, r0, #UART_RX_READY_BIT
    ret

uart_read8:
    // We assume interrupt was triggered and we know there is a byte
    ldb r0, [#UART_DATA]
    ret

#define IC_IRQ_BASE           0xE010
#define IC_IRQ_OFFSET_VECTOR  0x0
#define IC_IRQ_OFFSET_CPU     0x1
#define IC_IRQ_OFFSET_FLAGS   0x2
#define IC_IRQ_STRIDE         4

#define IC_FLAGS_ENABLE_MASK  0x1
#define IC_FLAGS_ENABLE_BIT   0

// r0 = irq number
// r1 = vector index
// r2 = cpuid
ic_map_irq:
    
    li r3, #IC_IRQ_STRIDE
    umul r0, r0, r3
    lea r3, [#IC_IRQ_BASE]
    add r0, r0, r3

    stb r1, [r0 + #IC_IRQ_OFFSET_VECTOR]
    stb r2, [r0 + #IC_IRQ_OFFSET_CPU]

    ldh r3, [r0 + #IC_IRQ_OFFSET_FLAGS]
    li r4, #IC_FLAGS_ENABLE_MASK
    or r3, r3, r4
    sth r3, [r0 + #IC_IRQ_OFFSET_FLAGS]

    ret
