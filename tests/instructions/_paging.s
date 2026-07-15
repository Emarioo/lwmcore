/*
    @TODO Test USER denied page faults.
       We probably do it for user tests (since user mode requires paging enabled)
*/

    call setup_vector_base

    li r0, 0x0
    li r1, 0x1F
    li r2, 0x0
    call map_page

    li r0, 0x1
    li r1, 0x1F
    li r2, 0x1
    call map_page

    li r0, 0x5
    li r1, 0x1F
    li r2, 0x4
    call map_page

    li r0, 0x6
    li r1, 0x1F
    li r2, 0x4
    call map_page
    
    li r0, 0xF
    li r1, 0x1F
    li r2, 0xF
    call map_page


    /*
        Test 1
            Mapping one physical page to two different virtual pages.
    */
    call enable_paging

    ldh r0, [0x5000]
    #TEST(0x99)
    ldh r0, [0x6000]
    #TEST(0x99)

    li r3, 0x200
    sth r3, [0x5002]
    ldh r0, [0x6002]
    #TEST(0x200)


    /*
        Test 2
            No page fault (because we disable it)
    */
    call disable_paging

    ldb r0, [0x7000]
    ldh r0, [hit_page_fault]
    #TEST(0)


    /*
        Test 3
            Page fault, not present
    */
    call enable_paging
    li r0, 0
    mtcr CRCAUSE, r0

    ldb r0, [0x7000] // Page fault

    mfcr r0, CRCAUSE
    li r1, 0x1F // Present, user, read, write, execute mask
    and r0, r0, r1
    #TEST(4) // page fault cause: not present, operation was read.

    li r0, 0
    sth r0, [hit_page_fault]

    /*
        Test 4
            Page fault, read denied
            No page fault on write
    */
    call disable_paging

    li r0, 0x7
    li r1, 0x1B
    li r2, 0x7
    call map_page

    call enable_paging
    li r0, 0
    mtcr CRCAUSE, r0

    ldb r0, [0x7000] // Page fault

    mfcr r0, CRCAUSE
    li r1, 0x1F // Present, user, read, write, execute mask
    and r0, r0, r1
    #TEST(5) // page fault cause: Page present, read bit set meaning READ denied.

    li r0, 0
    mtcr CRCAUSE, r0

    stb r0, [0x7000]
    
    mfcr r0, CRCAUSE
    #TEST(0) // No page fault on write


    /*
        Test 5
            Page fault, write denied
            No page fault on read
    */
    call disable_paging

    li r0, 0x7
    li r1, 0x17
    li r2, 0x7
    call map_page

    call enable_paging
    li r0, 0
    mtcr CRCAUSE, r0

    stb r0, [0x7000] // Page fault

    mfcr r0, CRCAUSE
    li r1, 0x1F // Present, user, read, write, execute mask
    and r0, r0, r1
    #TEST(9) // page fault cause: Page present, write bit set meaning WRITE denied.

    li r0, 0
    mtcr CRCAUSE, r0

    ldb r0, [0x7000]
    
    mfcr r0, CRCAUSE
    #TEST(0) // No page fault on read


    /*
        Test 5
            Page fault, execute denied
            No page fault on read
    */
    call disable_paging

    li r0, 0x7
    li r1, 0x0F
    li r2, 0x7
    call map_page

    call enable_paging
    li r0, 0
    mtcr CRCAUSE, r0

    li r0, 1
    sth r0, [page_fault_vret_to_lr]

    lea r0, [0x7000]
    call r0 // Page fault, note that we don't page fault on this instruction but when we start executing where we jumped too.

    li r0, 0
    sth r0, [page_fault_vret_to_lr]

    mfcr r0, CRCAUSE
    li r1, 0x1F // Present, user, read, write, execute mask
    and r0, r0, r1
    #TEST(0x11) // page fault cause: Page present, execute bit set meaning EXECUTE denied.

    li r0, 0
    mtcr CRCAUSE, r0

    ldb r0, [0x7000]
    
    mfcr r0, CRCAUSE
    #TEST(0) // No page fault on read


    jmp end_of_test

# ##########################################
#    Functions used by all 16/32/64 CPUs
# ##########################################

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
hit_page_fault:
    quad
page_fault_vret_to_lr:
    quad

ex_handler:
    hlt

ex_handler_page:
    mfcr sp, CRESP

    li r0, 1
    xadd r0, [hit_page_fault]

    ldb r0, [page_fault_vret_to_lr]
    jnz r0, .ret_lr

    mfcr r0, CREPC
    lea r0, [r0 + 4] // 'ldb r0, [0x7000]' is 4 bytes
    mtcr CREPC, r0

    jmp .end
.ret_lr:

    mtcr CREPC, lr

.end:
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

    #VECTOR(3, ex_handler_page)

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


end_of_test:
