
#define EMU_HALT     0xF000
#define EMU_OUT      0xF004

section .data 0x20

msg:
    byte[] "Hello, Sailor!\n\0"

section .text 0x0
main:
    li r1, 0
beg:
    ldb r0, [r1 + msg]
    jz r0, end
    call putchar
    li r0, 1
    add r1, r1, r0
    jmp beg
end:
    stb r0, [#EMU_HALT]
    // The pseudo instruction 'hlt' can be used instead

putchar:
    stb r0, [#EMU_OUT] 
    ret
