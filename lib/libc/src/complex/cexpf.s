        ;; cexpf.s
        ;;
        ;; libc cexpf() for the xcc Z80 libc.
        ;; Uses the identity
        ;;   exp(x + i y) = exp(x) * (cos(y) + i sin(y))
        ;; on top of the existing real-valued float helpers.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cexpf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cexpf
        .globl  _expf
        .globl  _cosf
        .globl  _sinf
        .globl  ___fsmul

        .area   _CODE

_cexpf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl

        ;; expx = expf(real(z))
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _expf
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h

        ;; real = expx * cosf(imag(z))
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _cosf
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h

        ;; imag = expx * sinf(imag(z))
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _sinf
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h

        ;; Return the packed complex result.
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        exx
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        exx

        ld      sp,ix
        pop     ix
        ret
