        ;; cpowf.s
        ;;
        ;; libc cpowf() for the xcc Z80 libc.
        ;; Uses the principal-value identity
        ;;   cpow(a, b) = cexp(b * clog(a))
        ;; with the product expanded as an ordinary complex multiply.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cpowf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cpowf
        .globl  _clogf
        .globl  _cexpf
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  ___fssub

        .area   _CODE

_cpowf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-24
        add     hl,sp
        ld      sp,hl

        ;; log(a)
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
        call    _clogf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      -24(ix),e
        ld      -23(ix),d
        ld      -22(ix),l
        ld      -21(ix),h
        exx
        ld      -20(ix),e
        ld      -19(ix),d
        ld      -18(ix),l
        ld      -17(ix),h
        exx

        ;; tmp0 = log_re * b_re
        ld      l,14(ix)
        ld      h,15(ix)
        push    hl
        ld      l,12(ix)
        ld      h,13(ix)
        push    hl
        ld      e,-24(ix)
        ld      d,-23(ix)
        ld      l,-22(ix)
        ld      h,-21(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -16(ix),e
        ld      -15(ix),d
        ld      -14(ix),l
        ld      -13(ix),h

        ;; tmp1 = log_im * b_im
        ld      l,18(ix)
        ld      h,19(ix)
        push    hl
        ld      l,16(ix)
        ld      h,17(ix)
        push    hl
        ld      e,-20(ix)
        ld      d,-19(ix)
        ld      l,-18(ix)
        ld      h,-17(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h

        ;; prod_re = tmp0 - tmp1
        ld      l,-10(ix)
        ld      h,-9(ix)
        push    hl
        ld      l,-12(ix)
        ld      h,-11(ix)
        push    hl
        ld      e,-16(ix)
        ld      d,-15(ix)
        ld      l,-14(ix)
        ld      h,-13(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h

        ;; tmp0 = log_re * b_im
        ld      l,18(ix)
        ld      h,19(ix)
        push    hl
        ld      l,16(ix)
        ld      h,17(ix)
        push    hl
        ld      e,-24(ix)
        ld      d,-23(ix)
        ld      l,-22(ix)
        ld      h,-21(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -16(ix),e
        ld      -15(ix),d
        ld      -14(ix),l
        ld      -13(ix),h

        ;; tmp1 = log_im * b_re
        ld      l,14(ix)
        ld      h,15(ix)
        push    hl
        ld      l,12(ix)
        ld      h,13(ix)
        push    hl
        ld      e,-20(ix)
        ld      d,-19(ix)
        ld      l,-18(ix)
        ld      h,-17(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h

        ;; prod_im = tmp0 + tmp1
        ld      l,-10(ix)
        ld      h,-9(ix)
        push    hl
        ld      l,-12(ix)
        ld      h,-11(ix)
        push    hl
        ld      e,-16(ix)
        ld      d,-15(ix)
        ld      l,-14(ix)
        ld      h,-13(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h

        ;; cexp(prod_re + i prod_im)
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        ld      l,-6(ix)
        ld      h,-5(ix)
        push    hl
        ld      l,-8(ix)
        ld      h,-7(ix)
        push    hl
        call    _cexpf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      sp,ix
        pop     ix
        ret
