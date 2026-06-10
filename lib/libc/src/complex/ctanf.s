        ;; ctanf.s
        ;;
        ;; libc ctanf() for the xcc Z80 libc.
        ;; Uses the stable split
        ;;   tan(x + i y) = sin(2x)/(cos(2x)+cosh(2y))
        ;;                + i*sinh(2y)/(cos(2x)+cosh(2y))
        ;; on top of the existing real float kernels.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module ctanf
        .optsdcc -mz80 sdcccall(1)

        .globl  _ctanf
        .globl  _sinf
        .globl  _cosf
        .globl  _sinhf
        .globl  _coshf
        .globl  ___fsadd
        .globl  ___fsdiv

        .area   _DATA
__ctanf_two_x:
        .ds     4
__ctanf_two_y:
        .ds     4
__ctanf_denom:
        .ds     4
__ctanf_real:
        .ds     4
__ctanf_imag:
        .ds     4

        .area   _CODE

_ctanf::
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
        ld      (__ctanf_two_x),de
        ld      (__ctanf_two_x + 2),hl

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
        ld      (__ctanf_two_y),de
        ld      (__ctanf_two_y + 2),hl

        ;; denom = cos(2x) + cosh(2y)
        ld      de,(__ctanf_two_x)
        ld      hl,(__ctanf_two_x + 2)
        call    _cosf
        ld      (__ctanf_denom),de
        ld      (__ctanf_denom + 2),hl
        ld      de,(__ctanf_two_y)
        ld      hl,(__ctanf_two_y + 2)
        call    _coshf
        ld      hl,(__ctanf_denom + 2)
        push    hl
        ld      hl,(__ctanf_denom)
        push    hl
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__ctanf_denom),de
        ld      (__ctanf_denom + 2),hl

        ;; real = sin(2x) / denom
        ld      de,(__ctanf_two_x)
        ld      hl,(__ctanf_two_x + 2)
        call    _sinf
        ld      hl,(__ctanf_denom + 2)
        push    hl
        ld      hl,(__ctanf_denom)
        push    hl
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      (__ctanf_real),de
        ld      (__ctanf_real + 2),hl

        ;; imag = sinh(2y) / denom
        ld      de,(__ctanf_two_y)
        ld      hl,(__ctanf_two_y + 2)
        call    _sinhf
        ld      hl,(__ctanf_denom + 2)
        push    hl
        ld      hl,(__ctanf_denom)
        push    hl
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      (__ctanf_imag),de
        ld      (__ctanf_imag + 2),hl

        ld      de,(__ctanf_real)
        ld      hl,(__ctanf_real + 2)
        exx
        ld      de,(__ctanf_imag)
        ld      hl,(__ctanf_imag + 2)
        exx
        pop     ix
        ret
