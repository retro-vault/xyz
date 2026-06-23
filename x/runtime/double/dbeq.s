        ; double equal — returns A = 1 if a==b else 0
        ; a in DE:HL:DE':HL', b at ix+4..ix+11
        ;
        ; Pops own return address into IY (which ___dbcmp preserves) so that
        ; the stack arg b lands at the offset ___dbcmp expects.
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module dbeq
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___dbeq
        .globl  ___dbcmp

        ; ___dbeq
        ; inputs:  a in DE:HL:DE':HL', b at ix+4..ix+11 (stack)
        ; outputs: A = 1 if a == b, else 0
        ; clobbers: af, bc, de, hl, ix, iy, de', hl'
___dbeq:
        pop     iy              ; IY = return address
        call    ___dbcmp        ; DE = -1/0/+1 (b now at ix+4)
        ld      a, d
        or      e
        jr      nz, .neq
        ld      a, #1
        jp      (iy)
.neq:
        xor     a
        jp      (iy)
