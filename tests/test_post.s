
#include "tests/put.s"

# Test framework

total_tests:
    long
passed_tests:
    long
coverage_vector:
    byte[#counter]
coverage_vector_size:
    long #counter

msg_passed:
    byte[] "Passed #\0"
msg_failed:
    byte[] "FAILED #\0"

test_eq:
    push lr
    push r3

    li r3, 1
    stb r3, [coverage_vector + r2]
    xadd r3, [total_tests]
    jne r0, r1, .failure

    xadd r3, [passed_tests]
    mov r3, r0
    lea r0, [msg_passed]
      call putstring

    jmp .end
.failure:
    mov r3, r0
    lea r0, [msg_failed]
      call putstring

.end:
    mov r0, r2
      call putint
    li r0, ' '
      call putchar
    mov r0, r3
      call putint
    li r0, '='
      call putchar
    mov r0, r1
      call putint
    li r0, '\n'
      call putchar

    pop r3
    pop lr
    ret

msg_coverage:
    byte[] "Test coverage: \0"
msg_result:
    byte[] "Test result: \0"

test_finish:
    push lr
    push r1
    push r2
    push r3
    push r4

    # Check test coverage

    #LOAD r3, [coverage_vector_size]
    li r1, 1
    sub r3, r3, r1

    li r4, 0
    li r2, 0
.loop:
    ldb r1, [coverage_vector + r2]

    # @TODO Print vector
    ; li r0, '0'
    ; add r0, r0, r1
    ; call putchar

    jz r1, .not_set
    li r1, 1
    add r4, r4, r1
.not_set:

    li r1, 1
    add r2, r2, r1
    jlt r2, r3, .loop

    ; li r0, '\n'
    ; call putchar

    # Check test result

    lea r0, [msg_coverage]
    call putstring

    mov r0, r4
    call putint
    li r0, '/'
    call putchar
    mov r0, r3
    call putint
    li r0, '\n'
    call putchar

    lea r0, [msg_result]
    call putstring

    #LOAD r0, [passed_tests]
    call putint
    li r0, '/'
    call putchar
    #LOAD r0, [total_tests]
    call putint
    li r0, '\n'
    call putchar

    pop r4
    pop r3
    pop r2
    pop r1
    pop lr
    ret
