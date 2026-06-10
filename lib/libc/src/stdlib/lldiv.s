        ;; lldiv.s
        ;;
        ;; libc lldiv() for the xcc Z80 libc.
        ;; xcc currently lowers 16-byte aggregate returns through a pointer-like
        ;; ABI, so this routine writes the result into a static lldiv_t cell and
        ;; returns its address in DE.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module lldiv
        .optsdcc -mz80 sdcccall(1)

        .globl  _lldiv
        .globl  __divsll
        .globl  __modsll

        .area   _DATA
__lldiv_result:
        .ds     16

        .area   _CODE

_lldiv::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; Numerator arrives on stack at ix+4..11, denominator at ix+12..19.
        ld      l,18(ix)
        ld      h,19(ix)
        push    hl
        ld      l,16(ix)
        ld      h,17(ix)
        push    hl
        ld      l,14(ix)
        ld      h,15(ix)
        push    hl
        ld      l,12(ix)
        ld      h,13(ix)
        push    hl
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        pop     de
        pop     hl
        exx
        pop     de
        pop     hl                      ; arg0 = numerator in DE:HL:DE':HL'
        exx
        call    __divsll
        pop     bc
        pop     bc
        pop     bc
        pop     bc

        ;; Quotient into bytes [0..7].
        ld      a,e
        ld      (__lldiv_result + 0),a
        ld      a,d
        ld      (__lldiv_result + 1),a
        ld      a,l
        ld      (__lldiv_result + 2),a
        ld      a,h
        ld      (__lldiv_result + 3),a
        exx
        ld      a,e
        ld      (__lldiv_result + 4),a
        ld      a,d
        ld      (__lldiv_result + 5),a
        ld      a,l
        ld      (__lldiv_result + 6),a
        ld      a,h
        ld      (__lldiv_result + 7),a
        exx

        ;; Rebuild the arguments for the remainder helper.
        ld      l,18(ix)
        ld      h,19(ix)
        push    hl
        ld      l,16(ix)
        ld      h,17(ix)
        push    hl
        ld      l,14(ix)
        ld      h,15(ix)
        push    hl
        ld      l,12(ix)
        ld      h,13(ix)
        push    hl
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        pop     de
        pop     hl
        exx
        pop     de
        pop     hl
        exx
        call    __modsll
        pop     bc
        pop     bc
        pop     bc
        pop     bc

        ld      a,e
        ld      (__lldiv_result + 8),a
        ld      a,d
        ld      (__lldiv_result + 9),a
        ld      a,l
        ld      (__lldiv_result + 10),a
        ld      a,h
        ld      (__lldiv_result + 11),a
        exx
        ld      a,e
        ld      (__lldiv_result + 12),a
        ld      a,d
        ld      (__lldiv_result + 13),a
        ld      a,l
        ld      (__lldiv_result + 14),a
        ld      a,h
        ld      (__lldiv_result + 15),a
        exx

        ld      de,#__lldiv_result
        pop     ix
        ret
