        ; double less-than — returns A = 1 if a<b else 0
        ; a in DE:HL:DE':HL', b at ix+4..ix+11
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module dblt
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___dblt
        .globl  ___dbcmp

        ; ___dblt
        ; inputs:  a in DE:HL:DE':HL', b at ix+4..ix+11 (stack)
        ; outputs: A = 1 if a < b, else 0
        ; clobbers: af, bc, de, hl, ix, iy, de', hl'
___dblt:
        pop     iy
        call    ___dbcmp        ; DE = -1 if a<b
        ld      a, d
        inc     a               ; DE==0xFFFF → D=0xFF, inc → 0x00
        jr      nz, .nlt
        ld      a, e
        inc     a               ; E==0xFF → 0x00
        jr      nz, .nlt
        ld      a, #1
        jp      (iy)
.nlt:
        xor     a
        jp      (iy)
