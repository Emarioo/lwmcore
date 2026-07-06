

UART


```arm

#define UART_STATUS   0xE100
#define UART_CONTROL  0xE104
#define UART_DATA     0xE108
#define UART_IRQ      0xE10C // Read-only

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
    shr r0, r0, #UART_RX_READY_BIT
    ret

uart_read8:
    # We assume interrupt was triggered and we know there is a byte
    ldb r0, [#UART_DATA]
    ret

```
