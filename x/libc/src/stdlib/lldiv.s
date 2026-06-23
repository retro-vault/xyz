        ;; lldiv.s
        ;;
        ;; libc lldiv() for the xcc Z80 libc.
        ;; xcc currently materializes 16-byte aggregate returns by having the
        ;; caller copy from a callee-returned pointer.  To keep that ABI while
        ;; avoiding shared writable scratch, this routine reuses the caller's
        ;; own 16-byte argument block as transient result storage and returns a
        ;; pointer to that stack region in DE.
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
        ld      hl,#-8
        add     hl,sp
        ld      sp,hl                   ; local quotient spill [ix-8..ix-1]

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

        ;; Quotient occupies bytes [0..7] of the caller-visible result.
        ld      a,-8(ix)
        ld      4(ix),a
        ld      a,-7(ix)
        ld      5(ix),a
        ld      a,-6(ix)
        ld      6(ix),a
        ld      a,-5(ix)
        ld      7(ix),a
        ld      a,-4(ix)
        ld      8(ix),a
        ld      a,-3(ix)
        ld      9(ix),a
        ld      a,-2(ix)
        ld      10(ix),a
        ld      a,-1(ix)
        ld      11(ix),a

        ;; Remainder occupies bytes [8..15].
        ld      a,e
        ld      12(ix),a
        ld      a,d
        ld      13(ix),a
        ld      a,l
        ld      14(ix),a
        ld      a,h
        ld      15(ix),a
        exx
        ld      a,e
        ld      16(ix),a
        ld      a,d
        ld      17(ix),a
        ld      a,l
        ld      18(ix),a
        ld      a,h
        ld      19(ix),a
        exx

        push    ix
        pop     hl
        ld      de,#4
        add     hl,de
        ex      de,hl                   ; DE = caller argument block / result
        ld      sp,ix
        pop     ix
        ret
