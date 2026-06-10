        ;; ldiv.s
        ;;
        ;; libc ldiv() for the xcc Z80 libc.
        ;; Computes quotient and remainder for two signed 32-bit longs and
        ;; returns ldiv_t as an 8-byte aggregate in the normal 64-bit
        ;; DE:HL:DE':HL' register layout.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module ldiv
        .optsdcc -mz80 sdcccall(1)

        .globl  _ldiv
        .globl  __sdiv32
        .globl  __smod32

        .area   _DATA
__ldiv_numer_lo:
        .dw     0
__ldiv_numer_hi:
        .dw     0
__ldiv_quot_lo:
        .dw     0
__ldiv_quot_hi:
        .dw     0

        .area   _CODE

_ldiv::
        ;; First arg numer arrives in DE:HL. The second long denom is stacked
        ;; at ix+4..7 (low word first) under sdcccall(1).
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__ldiv_numer_lo),de
        ld      (__ldiv_numer_hi),hl

        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl                      ; denom low
        ld      de,(__ldiv_numer_lo)
        ld      hl,(__ldiv_numer_hi)
        call    __sdiv32                ; DE:HL = quotient (lo:hi)
        ld      (__ldiv_quot_lo),de
        ld      (__ldiv_quot_hi),hl
        pop     af
        pop     af

        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        ld      de,(__ldiv_numer_lo)
        ld      hl,(__ldiv_numer_hi)
        call    __smod32                ; DE:HL = remainder (lo:hi)
        pop     af
        pop     af

        push    hl                      ; rem high
        push    de                      ; rem low
        ld      de,(__ldiv_quot_lo)     ; DE = quotient low
        ld      hl,(__ldiv_quot_hi)     ; HL = quotient high
        exx
        pop     de                      ; DE' = rem low
        pop     hl                      ; HL' = rem high
        exx
        pop     ix
        ret
