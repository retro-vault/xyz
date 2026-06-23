        ; wmemset.s — fill count wide elements with c.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wmemset
        .optsdcc -mz80 sdcccall(1)
        .globl  _wmemset
        .area   _CODE
        ; HL = dst, DE = c, 4(ix) = count -> DE = dst
_wmemset::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; save dst
        ld      c,4(ix)
        ld      b,5(ix)
wms_loop:
        ld      a,b
        or      c
        jr      z,wms_done
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        dec     bc
        jr      wms_loop
wms_done:
        pop     de
        pop     ix
        ret
