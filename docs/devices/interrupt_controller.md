
Interrupt Controller for LWM

```arm

#define IC_EOI                0xE000
#define IC_IRQ_BASE           0xE004
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

    ldh r3, [r0 + #IC_OFFSET_FLAGS]
    li r4, #IC_FLAGS_ENABLE_MASK
    or r3, r3, r4
    sth r3, [r0 + #IC_OFFSET_FLAGS]

    ret

ic_eoi:
    li r0, 1
    stb r0, [#IC_IRQ_EOI]
    ret

```
