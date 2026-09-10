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
        ;   2(sp)..3(sp) = byte count (callee-clean)
        ; outputs:
        ;   DE = original destination
        ; clobbers: AF, BC, HL; preserves IX and IY
_memcpy::
        pop     bc                      ; return address
        pop     af                      ; byte count, temporarily as raw AF
        push    bc                      ; retain the return address
        push    hl                      ; keep original destination for return
        push    af
        pop     bc                      ; BC = count; argument already removed
        ld      a,b
        or      c
        jr      z,memcpy_done
        ex      de,hl                   ; HL = source, DE = destination
        ldir                            ; bulk copy BC bytes forward
memcpy_done:
        pop     de
        ret
