
section .text 0x0
    jmp main

global_counter:
    long
core0_counter:
    long
core1_counter:
    long

main:
    li sp, 0x1000

    li r0, 1
    stb r0, [0xEFE2]
    lea r0, [core_entry]
    sth r0, [0xEFE8]
    li r0, 0x1
    stb r0, [0xEFE0]

  core0_loop:
    li r1, 1

    call lock
    ldh r0, [global_counter]
    add r0, r0, r1
    sth r0, [global_counter]
    call unlock

    ldh r0, [core0_counter]
    add r0, r0, r1
    sth r0, [core0_counter]

    li r2, 100
    jne r0, r2, core0_loop

    // Add some delay for core1 to finish counting
    li r1, 1
    li r0, 80
  delay:
    sub r0, r0, r1
    jnz r0, delay

    ldh r0, [global_counter]
    ldh r1, [core0_counter]
    ldh r2, [core1_counter]

    hlt

core_entry:
    li sp, 0x1100

  core1_loop:
    li r1, 1

    call lock
    ldh r0, [global_counter]
    add r0, r0, r1
    sth r0, [global_counter]
    call unlock

    ldh r0, [core1_counter]
    add r0, r0, r1
    sth r0, [core1_counter]

    li r2, 100
    jne r0, r2, core1_loop

  spin:
    jmp spin

    hlt

mutex_lock:
    long

lock:
    push r0
    push r1
    li r0, 0

  lock_loop:
    li r1, 1
    cas r0, r1, [mutex_lock]
    jne r0, r1, lock_loop

    pop r1
    pop r0
    ret

unlock:
    push r0
    li r0, 0
    stb r0, [mutex_lock]
    pop r0
    ret


putchar:
    stb r0, [0xF004] 

    stb r0, [r12 + 0xF008] 
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



