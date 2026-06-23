        ; fixed8_8_cmp.s
        ;
        ; Signed 8.8 compare.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_cmp
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_cmp

        .area   _CODE

        ; inputs:  HL = a, DE = b
        ; outputs: DE = -1 if a < b, 0 if a == b, +1 if a > b
_fixed8_8_cmp::
        ld      a,h
        xor     d
        jp      m,.sign_diff

        ; Same sign: unsigned byte compare gives the signed ordering.
        ld      a,h
        cp      d
        jr      nz,.same_high
        ld      a,l
        cp      e
        jr      z,.eq
        jr      c,.lt
        jr      .gt
.same_high:
        jr      c,.lt
        jr      .gt

.sign_diff:
        bit     7,h
        jr      nz,.lt
        jr      .gt

.eq:
        ld      de,#0
        ret
.lt:
        ld      de,#0xffff
        ret
.gt:
        ld      de,#1
        ret
