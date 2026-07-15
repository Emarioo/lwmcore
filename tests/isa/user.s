/*
    TEST_CONFIG = all
*/

#include "tests/test_pre.s"

#define CPUFEAT_REG_BYTES    0

#define CRSTATUS_USER        0x2
#define CRSTATUS_PAGING      0x4
#define CRSTATUS_INTERRUPT   0x8

#define PAGE_BIT_PRESENT     0x1
#define PAGE_BIT_USER        0x2
#define PAGE_BIT_READ        0x4
#define PAGE_BIT_WRITE       0x8
#define PAGE_BIT_EXECUTE     0x10

section .user 0x4000

user_main:
    
    /*
        Test 2
            Syscall
    */
    li r0, 8
    li r1, 5
    syscall
    mov r5, r1

    // r0 should be untouched
    #TEST(8)

    mov r0, r5 // syscall handler should add 10 to r1.
    #TEST(15)


    /*
        Test 3
            Access SUPERVISOR page (user bit not set)
    */
    li r0, 1
    sth r0, [check_page_fault]    

    ldb r0, [0x2000] // page fault

    li r0, 0
    sth r0, [check_page_fault]


    /*
        Test 4
            Exexute privileged instructions and check protection fault
    */
    li r0, 1
    stb r0, [check_privileged_instructions]

#define CHECK_PRIV(INST, TEMP_LABEL)
    li r0, 0
    stb r0, [hit_prot_fault]
    lea r9, [%TEMP_LABEL%]
    %INST%
%TEMP_LABEL%:
    ldb r0, [hit_prot_fault]
#endmacro
#define CHECK_PRIV2(INST, OP2, TEMP_LABEL)
    li r0, 0
    stb r0, [hit_prot_fault]
    lea r9, [%TEMP_LABEL%]
    %INST%, %OP2%
%TEMP_LABEL%:
    ldb r0, [hit_prot_fault]
#endmacro

    #CHECK_PRIV(tlbflush r0,     tmp0) #TEST(1)
    #CHECK_PRIV2(mfcr r12, cr12, tmp1) #TEST(1)
    #CHECK_PRIV2(mtcr cr12, r12, tmp2) #TEST(1)
    #CHECK_PRIV2(mscr r12, cr12, tmp3) #TEST(1)
    #CHECK_PRIV(vret,            tmp4) #TEST(1)
    #CHECK_PRIV(eint,            tmp5) #TEST(1)
    #CHECK_PRIV(dint,            tmp6) #TEST(1)
    #CHECK_PRIV(wfi,             tmp7) #TEST(1)
    #CHECK_PRIV(save r0,         tmp8) #TEST(1)
    #CHECK_PRIV(restore r0,      tmp9) #TEST(1)
    #CHECK_PRIV(advtimer r0,     tmpA) #TEST(1)

    # Test non-privileged instruction in case handler or macro is broken
    #CHECK_PRIV(rdtick r0,       tmpB) #TEST(0)

    jmp end_test
    hlt

align 8
    byte[256]
user_stack:

// Make sure user section doesn't go beyond 4K (we only map one page)
section .guard 0x5000
    byte

section .text 0

main:
    #TEST_PRE

    /*
        Preparations
    */
    li r0, 0x2000
    mtcr CRISP, r0

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

    li r0, 0x2
    li r1, 0x1D // user bit cleared
    li r2, 0x2
    call map_page

    li r0, 0x4
    li r1, 0x1F
    li r2, 0x4
    call map_page

    li r0, 0xF
    li r1, 0x1F
    li r2, 0xF
    call map_page

    
    /*
        Test 1
            Try to jump to user mode and fail if paging or interrupts are off.
    */
    lea r0, [user_stack]
    mtcr CRESP, r0
    lea r0, [end_test]
    mtcr CREPC, r0

    li r0, #CRSTATUS_USER
    li r1, #CRSTATUS_PAGING
    or r0, r0, r1
    mtcr CRESTATUS, r0
    vret

    li r0, #CRSTATUS_USER
    li r1, #CRSTATUS_INTERRUPT
    or r0, r0, r1
    mtcr CRESTATUS, r0
    vret

    ldh r0, [hit_prot_fault]
    #TEST(2)


    /*
        Jump to user mode
    */
    li r0, #CRSTATUS_USER
    li r1, #CRSTATUS_INTERRUPT
    or r0, r0, r1
    li r1, #CRSTATUS_PAGING
    or r0, r0, r1
    mtcr CRESTATUS, r0

    lea r0, [user_stack]
    mtcr CRESP, r0
    lea r0, [user_main]
    mtcr CREPC, r0

    vret

end_test:
    #TEST_POST
    hlt


enable_paging:
    mfcr r0, CRSTATUS
    li r1, #CRSTATUS_PAGING
    or r0, r0, r1
    mtcr CRSTATUS, r0
    ret

disable_paging:
    mfcr r0, CRSTATUS
    li r1, #CRSTATUS_PAGING
    not r1, r1
    and r0, r0, r1
    mtcr CRSTATUS, r0
    ret

align 8
hit_prot_fault:
    quad
check_page_fault:
    quad
check_privileged_instructions:
    quad

ex_handler:
    hlt

ex_handler_page:

    ldb r0, [check_page_fault]
    jz r0, .halt

    mfcr r0, CRCAUSE
    li r1, 0x1F // Present, user, read, write, execute mask
    and r0, r0, r1
    #TEST(0x7) // page fault cause: Page present, read operation, user bit set meaning USER denied.
    
    mfcr r0, CREPC
    lea r0, [r0 + 4] // skip 'ldb r0, [0x2000]' which is 4 bytes
    mtcr CREPC, r0
    vret

.halt:
    hlt

ex_handler_prot:
    li r1, 1
    xadd r1, [hit_prot_fault]

    ldb r0, [check_privileged_instructions]
    jz r0, .normal

    mtcr CREPC, r9

    jmp .end
.normal:
    mfcr r0, CREPC
    lea r0, [r0 + 1]
    mtcr CREPC, r0

.end:
    vret

ex_handler_syscall:
    // CPU switched to interrupt stack here
    push r0

    li r0, 10
    add r1, r1, r0
    
    pop r0
    vret

ex_handler_timer:
    /*
        We don't care about timer but we must handle it
        because we use 'advtimer' which may schedule timer handler.
        Of course we call it in user mode so we will get protection fault
        and never schedule but if it doesn't work as expected then we may schedule
        and accidently halt the test if we don't handle timer interrupt.
    */
    vret

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

    #VECTOR(2, ex_handler_prot)
    #VECTOR(3, ex_handler_page)
    #VECTOR(6, ex_handler_syscall)
    #VECTOR(9, ex_handler_timer)

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


#include "tests/test_post.s"


section .page_tables 0x8000
rootTable:
    quad[512]
pageTable2:
    quad[512]
pageTable1:
    quad[512]
pageDirectory:
    quad[512]
