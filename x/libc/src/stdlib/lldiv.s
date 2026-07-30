        ;; lldiv.s
        ;;
        ;; libc lldiv() for the xcc Z80 libc.
        ;; Writes the 16-byte lldiv_t result through the SDCC hidden aggregate
        ;; result pointer. No shared writable scratch is used.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module lldiv
        .optsdcc -mz80 sdcccall(1)

        .globl  _lldiv
        .globl  __divsll
        .globl  __modsll

        .area   _CODE

_lldiv::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl                   ; quotient [-8..-1], rem [-16..-9]

        ;; Hidden result pointer is at ix+4..5. Numerator follows at ix+6..13
        ;; and denominator at ix+14..21.
        ld      l,20(ix)
        ld      h,21(ix)
        push    hl
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

        ;; Spill quotient locally so the original arguments remain intact for
        ;; the remainder helper.  We later copy quotient+remainder into the
        ;; caller's outgoing argument block at ix+4..ix+19.
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h
        exx
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        exx

        ;; Rebuild the arguments for the remainder helper.
        ld      l,20(ix)
        ld      h,21(ix)
        push    hl
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

        ;; Spill the remainder beside the quotient before using BC as the
        ;; caller-result cursor (EXX would otherwise swap that cursor).
        ld      -16(ix),e
        ld      -15(ix),d
        ld      -14(ix),l
        ld      -13(ix),h
        exx
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h
        exx

        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,-8(ix)
        ld      (bc),a
        inc     bc
        ld      a,-7(ix)
        ld      (bc),a
        inc     bc
        ld      a,-6(ix)
        ld      (bc),a
        inc     bc
        ld      a,-5(ix)
        ld      (bc),a
        inc     bc
        ld      a,-4(ix)
        ld      (bc),a
        inc     bc
        ld      a,-3(ix)
        ld      (bc),a
        inc     bc
        ld      a,-2(ix)
        ld      (bc),a
        inc     bc
        ld      a,-1(ix)
        ld      (bc),a
        inc     bc
        ld      a,-16(ix)
        ld      (bc),a
        inc     bc
        ld      a,-15(ix)
        ld      (bc),a
        inc     bc
        ld      a,-14(ix)
        ld      (bc),a
        inc     bc
        ld      a,-13(ix)
        ld      (bc),a
        inc     bc
        ld      a,-12(ix)
        ld      (bc),a
        inc     bc
        ld      a,-11(ix)
        ld      (bc),a
        inc     bc
        ld      a,-10(ix)
        ld      (bc),a
        inc     bc
        ld      a,-9(ix)
        ld      (bc),a
        ld      sp,ix
        pop     ix
        ret
