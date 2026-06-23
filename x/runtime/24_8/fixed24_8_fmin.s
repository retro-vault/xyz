        ; fixed24_8_fmin.s
        ;
        ; Minimum for signed 24.8 fixed values.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_fmin
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_fmin

        .area   _CODE

        ; inputs:  DE:HL = x, stack = y
        ; outputs: DE:HL = min(x, y)
_fixed24_8_fmin::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        xor     7(ix)
        jp      m,.sign_diff
        ld      a,h
        cp      7(ix)
        jr      nz,.byte
        ld      a,l
        cp      6(ix)
        jr      nz,.byte
        ld      a,d
        cp      5(ix)
        jr      nz,.byte
        ld      a,e
        cp      4(ix)
        jr      c,.keep_x
        jr      .load_y
.byte:
        jr      c,.keep_x
        jr      .load_y
.sign_diff:
        bit     7,h
        jr      nz,.keep_x
.load_y:
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
.keep_x:
        pop     ix
        ret
