
#define UART_BASE 0xF004
#define LOG_BASE  0xF008

#define CRSTATUS_USER        0x2
#define CRSTATUS_PAGING      0x4
#define CRSTATUS_INTERRUPT   0x8

#define PAGE_BIT_PRESENT     0x1
#define PAGE_BIT_USER        0x2
#define PAGE_BIT_READ        0x4
#define PAGE_BIT_WRITE       0x8
#define PAGE_BIT_EXECUTE     0x10

#define CRCAUSE_PRESENT      0x1
#define CRCAUSE_USER         0x2
#define CRCAUSE_READ         0x4
#define CRCAUSE_WRITE        0x8
#define CRCAUSE_EXECUTE      0x10

section .page_tables 0x2000
rootTable:
    long[1024]

section .text 0x0
    jmp main

init_msg:
    byte[] "core0\n\0"

    

align 4
vector:
    long[12] #repeat 11 "ex_handler, " ex_handler

main:
    li sp, 0x1000
    mtcr CRISP, sp

    lea r0, [vector]
    mtcr CRVB, r0

    lea r0, [ex_handler_prot_fault]
    sth r0, [vector + 8]

    lea r0, [ex_handler_page_fault]
    sth r0, [vector + 12]

    lea r0, [ex_handler_double_fault]
    sth r0, [vector + 16]
    
    lea r0, [ex_handler_syscall]
    sth r0, [vector + 24]

    lea r0, [init_msg]
    call putstring

    lea r0, [rootTable]
    mtcr CRPT, r0


    /*
        Try not setting user bit for kernel pages which contain putstring, it should cause page fault user.
    */

#define KERNEL_FLAGS 0x1F
// #define KERNEL_FLAGS 0x1D
#define USER_FLAGS   0x1F

    li r0, 0x0
    li r1, #KERNEL_FLAGS
    li r2, 0x0
    call map_page
    
    li r0, 0x1
    li r1, #KERNEL_FLAGS
    li r2, 0x1
    call map_page

    li r0, 0x2
    li r1, #KERNEL_FLAGS
    li r2, 0x2
    call map_page
    
    li r0, 0x7
    li r1, #USER_FLAGS
    li r2, 0x7
    call map_page
    
    li r0, 0xF
    li r1, #USER_FLAGS
    li r2, 0xF
    call map_page

    /*
        Try entering user mode with paging and interrupts disabled, it should case protection fault
    */

    mfcr r0, CRSTATUS
    li r1, #CRSTATUS_PAGING
    or r0, r0, r1
    mtcr CRSTATUS, r0

    li r0, #CRSTATUS_USER
    li r1, #CRSTATUS_INTERRUPT
    or r0, r0, r1
    li r1, #CRSTATUS_PAGING
    or r0, r0, r1
    mtcr CRESTATUS, r0

    lea r0, [user_stack]
    mtcr CRESP, r0

    lea r0, [user_code]
    mtcr CREPC, r0

    vret

    hlt

// r0 = 1024-index into page table
// r1 = Flags
// r2 = Physical Address bits 12:21 (low 12 bits should be aligned to page and are not needed)
map_page:
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
    ret

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


page_fault_msg:
    byte[] "Page Fault, \0"
write_msg:
    byte[] "write\n\0"
user_msg:
    byte[] "user\n\0"
read_msg:
    byte[] "read\n\0"
exec_msg:
    byte[] "exec\n\0"
present_msg:
    byte[] "present\n\0"


ex_handler_page_fault:
    li r0, 0
    mtcr CRSTATUS, r0 // turn off paging and interrupts
    lea r0, [page_fault_msg]
    call putstring

    mfcr r6, CRCAUSE

    // If present flag is zero then that was the issue.
    // The appropriate read/write/exec flag is set to indicate which
    // operation was attempted when page wasn't present
    li r0, #CRCAUSE_PRESENT
    and r0, r0, r6
    jnz r0, is_present
    lea r0, [present_msg]
    call putstring
    jmp end
  is_present:

#define CHECK(BIT, MSG, LABEL)
    li r0, %BIT%
    and r0, r0, r6
    jz r0, %LABEL%
    lea r0, [%MSG%]
    call putstring
    jmp end
  %LABEL%:
#endmacro
    
    // User flag is set if page fault was caused by disallowed access to supervisor page.
    // If user flag is not set then that was not the issue. See CRESTATUS to know if 
    // user or supervisor got the page fault.

    // read/write/exec indicate which operation caused the page fault.

    #CHECK(0x2, user_msg, n0)
    #CHECK(0x4, read_msg, n1)
    #CHECK(0x8, write_msg, n2)
    #CHECK(0x10, exec_msg, n3)

  end:
    hlt

default_ex_msg:
    byte[] "Generic Fault\n\0"

ex_handler:
    li r0, 0
    mtcr CRSTATUS, r0 // turn off paging and interrupts
    lea r0, [default_ex_msg]
    call putstring
    hlt
    
prot_ex_msg:
    byte[] "Protection Fault\n\0"

ex_handler_prot_fault:
    li r0, 0
    mtcr CRSTATUS, r0 // turn off paging and interrupts
    lea r0, [prot_ex_msg]
    call putstring
    hlt

double_ex_msg:
    byte[] "Double Fault\n\0"

ex_handler_double_fault:
    li r0, 0
    mtcr CRSTATUS, r0 // turn off paging and interrupts
    lea r0, [double_ex_msg]
    call putstring
    hlt

syscall_msg:
    byte[] "syscall \0"

save_context:
    short[32]

ex_handler_syscall:
    sth r0, [save_context]
    lea r0, [save_context]
    save r0, 0

    lea r0, [syscall_msg]
    call putstring

    li r0, '0'
    ldb r1, [save_context]
    add r0, r0, r1
    call putchar
    
    li r0, ' '
    call putchar
    
    li r0, '0'
    ldb r1, [save_context + 2]
    add r0, r0, r1
    call putchar
    
    li r0, ' '
    call putchar
    
    li r0, '0'
    ldb r1, [save_context + 4]
    add r0, r0, r1
    call putchar
    
    li r0, '\n'
    call putchar

    lea r0, [save_context]
    restore r0, 0

    vret

section .user 0x7000

user_enter_msg:
    byte[] "User code\n\0"


user_code:

    lea r0, [user_enter_msg]
    call putstring

    // These syscalls show that we save context properly
    li r0, 3
    li r1, 5
    li r2, 7
    syscall
    
    add r2, r0, r1
    syscall

    li r0, 0
    mtcr CRSTATUS, r0 // protection fault on privileged instruction

    hlt

    byte[64]
user_stack:

