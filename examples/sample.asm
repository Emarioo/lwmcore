
include "examples/util.asm" 

main:
    li ra, 1
    shl ra, 8
    li ra, 1
    li rb, 2
    call add
    ret

var: int16 23
