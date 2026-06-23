        ; wmemmove.s — overlap-safe copy of count wide elements.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wmemmove
        .optsdcc -mz80 sdcccall(1)
        .globl  _wmemmove
        .area   _CODE
        ; HL = dst, DE = src, 4(ix) = count -> DE = dst
_wmemmove::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; save dst (return)
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,wmmv_done             ; count == 0
        sla     c
        rl      b                       ; BC = 2*count bytes
        ; dst < src ? forward copy
        ld      a,l
        sub     e
        ld      a,h
        sbc     d
        jr      c,wmmv_fwd
        ; backward: HL=dst, DE=src
        add     hl,bc
        dec     hl                      ; HL = dst_end
        ex      de,hl                   ; HL = src, DE = dst_end
        add     hl,bc
        dec     hl                      ; HL = src_end
        lddr
        jr      wmmv_done
wmmv_fwd:
        ex      de,hl                   ; HL = src, DE = dst
        ldir
wmmv_done:
        pop     de
        pop     ix
        ret
