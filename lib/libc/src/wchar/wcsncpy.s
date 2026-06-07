        ; wcsncpy.s — copy up to count elements, NUL-padding the remainder.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wcsncpy
        .optsdcc -mz80 sdcccall(1)
        .globl  _wcsncpy
        .area   _CODE
        ; HL = dst, DE = src, 4(ix) = count -> DE = dst
_wcsncpy::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; save dst
        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = count
wncp_copy:
        ld      a,b
        or      c
        jr      z,wncp_done
        ld      a,(de)                  ; low
        ld      (hl),a
        inc     de
        inc     hl
        ld      a,(de)                  ; high
        ld      (hl),a
        inc     de
        inc     hl
        dec     hl
        dec     hl
        or      (hl)                    ; A=high, (hl)=low -> Z if char == 0
        inc     hl
        inc     hl
        dec     bc
        jr      z,wncp_pad              ; copied a NUL -> pad the rest
        jr      wncp_copy
wncp_pad:
        ld      a,b
        or      c
        jr      z,wncp_done
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        dec     bc
        jr      wncp_pad
wncp_done:
        pop     de
        pop     ix
        ret
