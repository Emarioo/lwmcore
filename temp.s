
    li r0, 1
    lis r1, -1

    jb r0, r1, jb_taken
    li r0, 0

jb_taken:

hlt