        ; memchr.s
        ;
        ; libc memchr implementation for the xcc Z80 libc.
        ; Walks a byte span until either the target byte is found or the count
        ; runs out.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module memchr
        .optsdcc -mz80 sdcccall(1)


        .globl  _memchr
        .globl  __string_return_zero_clean2
        .globl  __string_return_hl_clean2

        .area   _CODE

        ; _memchr
        ; inputs:
        ;   HL         = start of span
        ;   DE         = search byte (low byte E is used)
        ;   4(ix)..5(ix) = byte count
        ; outputs:
        ;   DE = pointer to the first matching byte, or 0 if not found
        ; clobbers: AF, BC, HL, IX
_memchr::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
memchr_loop:
        ld      a,b
        or      c
        jr      z,memchr_not_found
        ld      a,(hl)
        cp      e
        jr      z,memchr_found
        inc     hl
        dec     bc
        jr      memchr_loop
memchr_found:
        pop     ix
        jp      __string_return_hl_clean2
memchr_not_found:
        pop     ix
        jp      __string_return_zero_clean2
