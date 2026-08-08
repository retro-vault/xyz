        ; memset.s
        ;
        ; libc memset implementation for the xcc Z80 libc.
        ; Seeds the first byte, then uses the classic overlap-fill trick:
        ; copy the initialized prefix onto the next byte with LDIR until the
        ; whole span is covered.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module memset
        .optsdcc -mz80 sdcccall(1)


        .globl  _memset

        .area   _CODE

        ; _memset
        ; inputs:
        ;   HL         = destination
        ;   DE         = fill value (low byte E is used)
        ;   4(ix)..5(ix) = byte count
        ; outputs:
        ;   DE = original destination
        ; clobbers: AF, BC, HL, IX
_memset::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; preserve original destination
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,memset_done
        ld      (hl),e                  ; seed byte 0 of the destination
        dec     bc                      ; BC now tracks the remaining tail
        ld      a,b
        or      c
        jr      z,memset_done
        ld      d,h
        ld      e,l
        inc     de                      ; DE = destination + 1
        ldir                            ; replicate the initialized prefix
memset_done:
        pop     de
        pop     ix
        ; sdcccall(1) returns of at most 16 bits are callee-clean.  Preserve
        ; the pointer result in DE while removing the spilled byte-count word.
        pop     hl                      ; return address
        pop     bc                      ; byte count
        jp      (hl)
