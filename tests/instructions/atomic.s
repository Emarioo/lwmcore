/*
    TEST_CONFIG = all
*/

#include "tests/test_pre.s"

#include "tests/platform.s"

section .text 0x0
main:
    #TEST_PRE

    call test_no_atomic
    call test_xadd
    call test_cas

    #TEST_POST
    hlt
 
test_no_atomic:
    push lr

    li r0, 0
    sth r0, [global_counter]

    lea r0, [test_no_atomic_core1]
    call boot_second_core

    li r3, 0

.core0_loop:
    li r1, 1
    
    ldh r0, [global_counter]
    add r0, r0, r1
    sth r0, [global_counter]

    add r3, r3, r1

    li r2, 100
    jlt r3, r2, .core0_loop

    // Add some delay for core1 to finish counting
    li r1, 1
    li r0, 50
.delay:
    sub r0, r0, r1
    jnz r0, .delay

    ldh r0, [global_counter]
    #TEST_NE(200)

    call reset_second_core

    pop lr
    ret


test_no_atomic_core1:
    li sp, 0x1100

    li r3, 0

.core1_loop:
    li r1, 1

    ldh r0, [global_counter]
    add r0, r0, r1
    sth r0, [global_counter]

    add r3, r3, r1

    li r2, 100
    jne r3, r2, .core1_loop

.spin:
    jmp .spin
    hlt



test_xadd:
    push lr

    li r0, 0
    sth r0, [global_counter]

    lea r0, [test_xadd_core1]
    call boot_second_core

    li r3, 0

.core0_loop:
    li r1, 1
    
    xadd r1, [global_counter]

    add r3, r3, r1

    li r2, 100
    jne r3, r2, .core0_loop

    // Add some delay for core1 to finish counting
    li r1, 1
    li r0, 50
.delay:
    sub r0, r0, r1
    jnz r0, .delay

    ldh r0, [global_counter]
    #TEST(200)

    call reset_second_core

    pop lr
    ret


test_xadd_core1:
    li sp, 0x1100

    li r3, 0
.core1_loop:
    li r1, 1

    xadd r1, [global_counter]

    add r3, r3, r1

    li r2, 100
    jne r3, r2, .core1_loop

.spin:
    jmp .spin
    hlt


test_cas:
    push lr

    li r0, 0
    sth r0, [global_counter]

    lea r0, [test_cas_core1]
    call boot_second_core

    li r3, 0

.core0_loop:
    li r1, 1
    
    call lock
    ldh r0, [global_counter]
    add r0, r0, r1
    sth r0, [global_counter]
    call unlock

    add r3, r3, r1

    li r2, 100
    jne r3, r2, .core0_loop

    // Add some delay for core1 to finish counting
    li r1, 1
    li r0, 50
.delay:
    sub r0, r0, r1
    jnz r0, .delay

    ldh r0, [global_counter]
    #TEST(200)

    call reset_second_core

    pop lr
    ret


test_cas_core1:
    li sp, 0x1100

    li r3, 0
.core1_loop:
    li r1, 1

    call lock
    ldh r0, [global_counter]
    add r0, r0, r1
    sth r0, [global_counter]
    call unlock

    add r3, r3, r1

    li r2, 100
    jne r3, r2, .core1_loop

.spin:
    jmp .spin
    hlt



/*
    void boot_second_core(void* entry);
        r0 = entry
*/
boot_second_core:
    ; lea r0, [core_entry]
    sth r0, [#PLATFORM_CORE_ENTRY]
    li r0, 1
    stb r0, [#PLATFORM_CORE_CPUID]
    li r0, #PLATFORM_CORE_CONTROL_BOOT
    stb r0, [#PLATFORM_CORE_CONTROL]
    ret

/*
    void reset_second_core(void);
*/
reset_second_core:
    li r0, 1
    stb r0, [#PLATFORM_CORE_CPUID]
    li r0, #PLATFORM_CORE_CONTROL_RESET
    stb r0, [#PLATFORM_CORE_CONTROL]
    ret


lock:
    push r0
    push r1
    li r0, 0

  .loop:
    li r1, 1
    cas r0, r1, [mutex_lock]
    jne r0, r1, .loop

    pop r1
    pop r0
    ret

unlock:
    push r0
    li r0, 0
    stb r0, [mutex_lock]
    pop r0
    ret

align 8
mutex_lock:
    quad
global_counter:
    quad


#include "tests/test_post.s"
