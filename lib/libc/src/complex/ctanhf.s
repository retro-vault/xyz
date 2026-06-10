        ;; ctanhf.s
        ;;
        ;; libc ctanhf() for the xcc Z80 libc.
        ;; Uses the stable split
        ;;   tanh(x + i y) = sinh(2x)/(cosh(2x)+cos(2y))
        ;;                 + i*sin(2y)/(cosh(2x)+cos(2y))
        ;; on top of the existing real float kernels.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module ctanhf
        .optsdcc -mz80 sdcccall(1)

        .globl  _ctanhf
        .globl  _sinf
        .globl  _cosf
        .globl  _sinhf
        .globl  _coshf
        .globl  ___fsadd
        .globl  ___fsdiv

        .area   _DATA
__ctanhf_two_x:
        .ds     4
__ctanhf_two_y:
        .ds     4
__ctanhf_denom:
        .ds     4
__ctanhf_real:
        .ds     4
__ctanhf_imag:
        .ds     4

        .area   _CODE

_ctanhf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; 2x = x + x
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__ctanhf_two_x),de
        ld      (__ctanhf_two_x + 2),hl

        ;; 2y = y + y
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__ctanhf_two_y),de
        ld      (__ctanhf_two_y + 2),hl

        ;; denom = cosh(2x) + cos(2y)
        ld      de,(__ctanhf_two_x)
        ld      hl,(__ctanhf_two_x + 2)
        call    _coshf
        ld      (__ctanhf_denom),de
        ld      (__ctanhf_denom + 2),hl
        ld      de,(__ctanhf_two_y)
        ld      hl,(__ctanhf_two_y + 2)
        call    _cosf
        ld      hl,(__ctanhf_denom + 2)
        push    hl
        ld      hl,(__ctanhf_denom)
        push    hl
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__ctanhf_denom),de
        ld      (__ctanhf_denom + 2),hl

        ;; real = sinh(2x) / denom
        ld      de,(__ctanhf_two_x)
        ld      hl,(__ctanhf_two_x + 2)
        call    _sinhf
        ld      hl,(__ctanhf_denom + 2)
        push    hl
        ld      hl,(__ctanhf_denom)
        push    hl
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      (__ctanhf_real),de
        ld      (__ctanhf_real + 2),hl

        ;; imag = sin(2y) / denom
        ld      de,(__ctanhf_two_y)
        ld      hl,(__ctanhf_two_y + 2)
        call    _sinf
        ld      hl,(__ctanhf_denom + 2)
        push    hl
        ld      hl,(__ctanhf_denom)
        push    hl
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      (__ctanhf_imag),de
        ld      (__ctanhf_imag + 2),hl

        ld      de,(__ctanhf_real)
        ld      hl,(__ctanhf_real + 2)
        exx
        ld      de,(__ctanhf_imag)
        ld      hl,(__ctanhf_imag + 2)
        exx
        pop     ix
        ret
