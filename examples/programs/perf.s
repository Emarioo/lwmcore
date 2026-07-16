
#include "tests/platform.s"

section .text 0

main:
    li sp, 0x1000

    rdtick r8

    li r4, 0
    li r1, 1
.loop:
    add r4, r4, r1

    ; slow # The execution time of the program should still be realtime if
           # we slow it down. For example if we run with and without slow we
           # should get different execution time.

    li r5, 4000
    jeq r4, r5, .end 
    mov r0, r4
        call putint
    li r0, '\n'
        call putchar

    jmp .loop
.end:

    ldl r3, [#PLATFORM_CORE_TICK_FREQ]
    rdtick r1

    sub r1, r1, r8
    li r2, 1000
    umul r1, r1, r2
    udiv r0, r1, r3

    // Print execution time.
    // In an emulator it would be how long the emulation took in realtime, including host process scheduling.
    // Not how long it took from the CPUs perspective disregarding emulator and process scheduling of the host.
    call putint

    li r0, ' ' call putchar
    li r0, 'm' call putchar
    li r0, 's' call putchar
    li r0, '\n' call putchar

    hlt

#include "tests/put.s"
