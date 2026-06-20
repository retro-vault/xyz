        ;; csqrtf.s
        ;;
        ;; libc csqrtf() for the xcc Z80 libc.
        ;;
        ;; Principal square root using the stable split:
        ;;   if x >= 0:
        ;;     real = sqrt((|z| + x) / 2)
        ;;     imag = y / (2 * real)
        ;;   else:
        ;;     imag = copysign(sqrt((|z| - x) / 2), y)
        ;;     real = |y| / (2 * |imag|)
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module csqrtf
        .optsdcc -mz80 sdcccall(1)

        .globl  _csqrtf
        .globl  _cabsf
        .globl  _fabsf
        .globl  _copysignf
        .globl  _sqrtf
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsmul
        .globl  ___fsdiv

        .area   _CODE

_csqrtf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl

        ;; |z| is shared by both branches.
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
        call    _cabsf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      -16(ix),e
        ld      -15(ix),d
        ld      -14(ix),l
        ld      -13(ix),h

        ;; z == 0 returns the original signed zeros unchanged.
        ld      a,d
        or      e
        or      h
        or      l
        jr      nz,csqrtf_nonzero
        ld      e,-16(ix)
        ld      d,-15(ix)
        ld      l,-14(ix)
        ld      h,-13(ix)
        exx
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        exx
        ld      sp,ix
        pop     ix
        ret

csqrtf_nonzero:
        bit     7,7(ix)
        jp      z,csqrtf_real_nonneg

        ;; imag = copysignf(sqrt((|z| - x) / 2), y)
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        ld      e,-16(ix)
        ld      d,-15(ix)
        ld      l,-14(ix)
        ld      h,-13(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        call    _sqrtf
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    _copysignf
        pop     bc
        pop     bc
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h

        ;; real = fabsf(y) / (2 * |imag|)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _fabsf
        ld      -4(ix),e               ; numerator
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      hl,#0x4000              ; 2.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h
        jp      csqrtf_done

csqrtf_real_nonneg:
        ;; real = sqrt((|z| + x) / 2)
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
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
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        call    _sqrtf
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h

        ;; imag = y / (2 * real)
        ld      hl,#0x4000              ; 2.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h

csqrtf_done:
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        exx
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        exx
        ld      sp,ix
        pop     ix
        ret
