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
        ;   2(sp)..3(sp) = byte count (callee-clean)
        ; outputs:
        ;   DE = original destination
        ; clobbers: AF, BC, HL; preserves IX and IY
_memset::
        pop     bc                      ; return address
        pop     af                      ; byte count, temporarily as raw AF
        push    bc                      ; retain the return address
        push    hl                      ; preserve original destination
        push    af
        pop     bc                      ; BC = count; argument already removed
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
        ret
