        ; memchr.s
        ;
        ; libc memchr implementation for the xcc Z80 libc.
        ; Uses the Z80 bounded byte-search instruction. CPIR stops at the
        ; first equal byte or when BC reaches zero, without reading ahead.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module memchr
        .optsdcc -mz80 sdcccall(1)


        .globl  _memchr

        .area   _CODE

        ; _memchr
        ; inputs:
        ;   HL         = start of span
        ;   DE         = search byte (low byte E is used)
        ;   2(sp)..3(sp) = byte count (caller-clean library ABI)
        ; outputs:
        ;   DE = pointer to the first matching byte, or 0 if not found
        ; clobbers: AF, BC, HL; preserves IX and IY
_memchr::
        pop     bc                      ; return address
        pop     af                      ; raw count, including its low byte
        push    af                      ; leave caller's argument in place
        push    bc
        push    af
        pop     bc                      ; BC = byte count
        ld      a,b
        or      c
        jr      z,memchr_not_found
        ld      a,e
        cpir
        jr      nz,memchr_not_found
        dec     hl                      ; CPIR advanced past the matching byte
        ex      de,hl
        ret
memchr_not_found:
        ld      de,#0
        ret
