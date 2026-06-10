        ;; casinhf.s
        ;;
        ;; libc casinhf() for the xcc Z80 libc.
        ;;
        ;; Principal-value inverse hyperbolic sine via
        ;;   asinh(z) = clog(z + csqrt(z*z + 1))
        ;; built from the existing real float helpers plus csqrtf()/clogf().
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module casinhf
        .optsdcc -mz80 sdcccall(1)

        .globl  _casinhf
        .globl  _csqrtf
        .globl  _clogf
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  ___fssub

        .area   _DATA
__casinhf_x2:
        .ds     4
__casinhf_y2:
        .ds     4
__casinhf_sqrt_re:
        .ds     4
__casinhf_sqrt_im:
        .ds     4
__casinhf_arg_re:
        .ds     4
__casinhf_arg_im:
        .ds     4
__casinhf_tmp:
        .ds     4

        .area   _CODE

_casinhf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; asinh(0) is exactly 0, and short-circuiting here avoids feeding the
        ;; soft-float log kernel its most cancellation-prone identity case.
        ld      a,4(ix)
        or      5(ix)
        or      6(ix)
        or      7(ix)
        or      8(ix)
        or      9(ix)
        or      10(ix)
        or      11(ix)
        jr      nz,casinhf_nonzero
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        pop     ix
        ret

casinhf_nonzero:

        ;; x^2
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        push    de
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__casinhf_x2),de
        ld      (__casinhf_x2 + 2),hl

        ;; y^2
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        push    de
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__casinhf_y2),de
        ld      (__casinhf_y2 + 2),hl

        ;; real(z*z + 1) = x^2 - y^2 + 1
        ld      hl,(__casinhf_y2 + 2)
        push    hl
        ld      hl,(__casinhf_y2)
        push    hl
        ld      de,(__casinhf_x2)
        ld      hl,(__casinhf_x2 + 2)
        call    ___fssub
        pop     bc
        pop     bc
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__casinhf_arg_re),de
        ld      (__casinhf_arg_re + 2),hl

        ;; imag(z*z + 1) = 2*x*y
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        push    de
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      hl,#0x4000              ; 2.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__casinhf_arg_im),de
        ld      (__casinhf_arg_im + 2),hl

        ;; s = csqrtf(z*z + 1)
        ld      hl,(__casinhf_arg_im + 2)
        push    hl
        ld      hl,(__casinhf_arg_im)
        push    hl
        ld      hl,(__casinhf_arg_re + 2)
        push    hl
        ld      hl,(__casinhf_arg_re)
        push    hl
        call    _csqrtf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      (__casinhf_sqrt_re),de
        ld      (__casinhf_sqrt_re + 2),hl
        exx
        ld      (__casinhf_sqrt_im),de
        ld      (__casinhf_sqrt_im + 2),hl
        exx

        ;; arg = z + s
        ld      hl,(__casinhf_sqrt_re + 2)
        push    hl
        ld      hl,(__casinhf_sqrt_re)
        push    hl
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__casinhf_tmp),de
        ld      (__casinhf_tmp + 2),hl

        ld      hl,(__casinhf_sqrt_im + 2)
        push    hl
        ld      hl,(__casinhf_sqrt_im)
        push    hl
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__casinhf_arg_im),de
        ld      (__casinhf_arg_im + 2),hl

        ;; clogf(arg)
        ld      hl,(__casinhf_arg_im + 2)
        push    hl
        ld      hl,(__casinhf_arg_im)
        push    hl
        ld      hl,(__casinhf_tmp + 2)
        push    hl
        ld      hl,(__casinhf_tmp)
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

        pop     ix
        ret
