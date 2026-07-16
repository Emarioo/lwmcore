/*
    We enable paging to get more realistic measurements of a user application.
*/

#include "tests/platform.s"
#include "tests/display.s"


#define CPUFEAT_REG_BYTES    0

#define CRSTATUS_USER        0x2
#define CRSTATUS_PAGING      0x4
#define CRSTATUS_INTERRUPT   0x8

#define PAGE_BIT_PRESENT     0x1
#define PAGE_BIT_USER        0x2
#define PAGE_BIT_READ        0x4
#define PAGE_BIT_WRITE       0x8
#define PAGE_BIT_EXECUTE     0x10

section .text 0

main:
    li sp, 0x1000


    ldl r6, [#DISPLAY_STRIDE]
    ldl r7, [#DISPLAY_WIDTH]
    ldl r8, [#DISPLAY_HEIGHT]
    umul r6, r6, r8

    call setup_vector_base
    call setup_page_tables

    li r0, 0x0
    li r1, 0x1F
    li r2, 0x0
    call map_page

    li r0, 0x1
    li r1, 0x1F
    li r2, 0x1
    call map_page

    li r0, 0xF
    li r1, 0x1F
    li r2, 0xF
    call map_page

    li r3, 0xD0
    li r4, 4096
    udiv r4, r6, r4 // frameBufferSize / 4096 = 75
    add r4, r4, r3 // 0xD0 + 75
    li r1, 1
    add r4, r4, r1 // an extra since framebuffer is at 0xD0040 and ends at 0x11b040 which is not page aligned.
iter:
    mov r0, r3
    li r1, 0x1F
    mov r2, r3
    call map_page
    li r1, 1
    add r3, r3, r1
    jne r3, r4, iter

    call enable_paging

    li r0, 0
    li r1, 4
    li r2, 0
.loop:
    stl r0, [#DISPLAY_FRAMEBUFFER + r2]
    add r2, r2, r1
    jeq r2, r6, .end
    jmp .loop
.end:

; Uncommenting gives raylib time to fully render the display.
;     li r0, 100
;     li r1, 1
; .spin:
;     slow
;     sub r0, r0, r1
;     jnz r0, .spin

    hlt


enable_paging:
    push r0
    push r1
    mfcr r0, CRSTATUS
    li r1, #CRSTATUS_PAGING
    or r0, r0, r1
    mtcr CRSTATUS, r0
    pop r1
    pop r0
    ret

disable_paging:
    push r0
    push r1
    mfcr r0, CRSTATUS
    li r1, #CRSTATUS_PAGING
    not r1, r1
    and r0, r0, r1
    mtcr CRSTATUS, r0
    pop r1
    pop r0
    ret

setup_page_tables:

    li r1, #CPUFEAT_REG_BYTES
    cpufeat r0, r1
    li r1, 2
    jne r0, r1, .try_setup32

; .setup16:

    lea r0, [rootTable]
    mtcr CRPT, r0

    jmp .end
.try_setup32:
    li r1, 4
    jne r0, r1, .setup64

; .setup32:
    
    lea r0, [pageTable1]
    li r1, 0x1F
    or r0, r0, r1
    stl r0, [rootTable]

    lea r0, [rootTable]
    mtcr CRPT, r0

    jmp .end
.setup64:
    
    lea r0, [pageDirectory]
    li r1, 0x1F
    or r0, r0, r1
    stl r0, [pageTable1]

    lea r0, [pageTable1]
    li r1, 0x1F
    or r0, r0, r1
    stl r0, [pageTable2]
    
    lea r0, [pageTable2]
    li r1, 0x1F
    or r0, r0, r1
    stl r0, [rootTable]

    lea r0, [rootTable]
    mtcr CRPT, r0

.end:

    ret

// r0 = 512-index into page table
// r1 = Flags
// r2 = Physical Address bits 12:47 (low 12 bits should be aligned to page and are not needed)
map_page:
    push r3
    push r4

    li r3, #CPUFEAT_REG_BYTES
    cpufeat r3, r3
    li r4, 2
    jne r3, r4, .try_map32

; .map16:

    mov r3, r2
    li r4, 12
    shl r2, r2, r4
    li r4, 4
    shr r3, r3, r4

    or r2, r2, r1

    li r4, 4
    umul r0, r0, r4
    sth r2, [rootTable + r0]
    sth r3, [rootTable + r0 + 2]

    jmp .end
.try_map32:
    
    li r4, 4
    jne r3, r4, .map64

; .map32:

    li r4, 12
    shl r2, r2, r4
    or r2, r2, r1

    li r4, 4
    umul r0, r0, r4
    stl r2, [pageTable1 + r0]

    jmp .end

.map64:

    li r4, 12
    shl r2, r2, r4
    or r2, r2, r1

    li r4, 8
    umul r0, r0, r4
    stl r2, [pageDirectory + r0]

.end:

    pop r4
    pop r3
    ret


ex_handler:
    hlt

setup_vector_base:
    push lr
    push r0
    push r1

    li r1, 0
    cpufeat r0, r1
    li r1, 8
    jeq r0, r1, .setup64

    lea r0, [vectors32]
    mtcr CRVB, r0

    jmp .end
.setup64:
    
    lea r0, [vectors64]
    mtcr CRVB, r0
.end:

    #define VECTOR(INDEX, HANDLER)
        li r0, %INDEX%
        lea r1, [%HANDLER%]
        call set_handler
    #endmacro

    ; #VECTOR(2, ex_handler_prot)
    ; #VECTOR(3, ex_handler_page)
    ; #VECTOR(6, ex_handler_syscall)
    ; #VECTOR(9, ex_handler_timer)

    pop r1
    pop r0
    pop lr
    ret

/*
    r0 = vector index
    r1 = handler
*/
set_handler:
    push r2

    li r2, 4
    umul r2, r2, r0
    sth r1, [vectors32 + r2]

    li r2, 8
    umul r2, r2, r0
    sth r1, [vectors64 + r2]

    pop r2
    ret

align 0x10
vectors32:
    long[12] #repeat 11 "ex_handler," ex_handler

align 0x10
vectors64:
    quad[12] #repeat 11 "ex_handler," ex_handler


#include "tests/put.s"

section .page_tables 0x8000
rootTable:
    quad[512]
pageTable2:
    quad[512]
pageTable1:
    quad[512]
pageDirectory:
    quad[512]
