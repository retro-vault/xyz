        ; memcpy.s
        ;
        ; libc memcpy implementation for the xcc Z80 libc.
        ; Uses a straight LDIR because memcpy has undefined behaviour on
        ; overlapping ranges, so the fast forward copy is sufficient.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module memcpy
        .optsdcc -mz80 sdcccall(1)


        .globl  _memcpy

        .area   _CODE

        ; _memcpy
        ; inputs:
        ;   HL         = destination
        ;   DE         = source
        ;   4(ix)..5(ix) = byte count
        ; outputs:
        ;   DE = original destination
        ; clobbers: AF, BC, HL, IX
_memcpy::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; keep original destination for return
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,memcpy_done
        ex      de,hl                   ; HL = source, DE = destination
        ldir                            ; bulk copy BC bytes forward
memcpy_done:
        pop     de
        pop     ix
        ; sdcccall(1) returns of at most 16 bits are callee-clean.  Preserve
        ; the pointer result in DE while removing the spilled byte-count word.
        pop     hl                      ; return address
        pop     bc                      ; byte count
        jp      (hl)
