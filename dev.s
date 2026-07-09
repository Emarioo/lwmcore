


main:

    li r0, 0
    li r1, 2
    cas r0, r1, [global]

    ldh r2, [global]

    hlt

global:
    long
