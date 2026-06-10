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

        .area   _DATA
__csqrtf_absz:
        .ds     4
__csqrtf_real:
        .ds     4
__csqrtf_imag:
        .ds     4
__csqrtf_tmp:
        .ds     4

        .area   _CODE

_csqrtf::
        push    ix
        ld      ix,#0
        add     ix,sp

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
        ld      (__csqrtf_absz),de
        ld      (__csqrtf_absz + 2),hl

        ;; z == 0 returns the original signed zeros unchanged.
        ld      a,d
        or      e
        or      h
        or      l
        jr      nz,csqrtf_nonzero
        ld      de,(__csqrtf_absz)
        ld      hl,(__csqrtf_absz + 2)
        exx
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        exx
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
        ld      de,(__csqrtf_absz)
        ld      hl,(__csqrtf_absz + 2)
        call    ___fssub
        pop     bc
        pop     bc
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        call    _sqrtf
        ld      (__csqrtf_tmp),de
        ld      (__csqrtf_tmp + 2),hl
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      de,(__csqrtf_tmp)
        ld      hl,(__csqrtf_tmp + 2)
        call    _copysignf
        pop     bc
        pop     bc
        ld      (__csqrtf_imag),de
        ld      (__csqrtf_imag + 2),hl

        ;; real = fabsf(y) / (2 * |imag|)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _fabsf
        ld      (__csqrtf_tmp),de       ; numerator
        ld      (__csqrtf_tmp + 2),hl
        ld      hl,#0x4000              ; 2.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__csqrtf_imag)
        ld      hl,(__csqrtf_imag + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      de,(__csqrtf_tmp)
        ld      hl,(__csqrtf_tmp + 2)
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      (__csqrtf_real),de
        ld      (__csqrtf_real + 2),hl
        jr      csqrtf_done

csqrtf_real_nonneg:
        ;; real = sqrt((|z| + x) / 2)
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        ld      de,(__csqrtf_absz)
        ld      hl,(__csqrtf_absz + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        call    _sqrtf
        ld      (__csqrtf_real),de
        ld      (__csqrtf_real + 2),hl

        ;; imag = y / (2 * real)
        ld      hl,#0x4000              ; 2.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      de,(__csqrtf_real)
        ld      hl,(__csqrtf_real + 2)
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
        ld      (__csqrtf_imag),de
        ld      (__csqrtf_imag + 2),hl

csqrtf_done:
        ld      de,(__csqrtf_real)
        ld      hl,(__csqrtf_real + 2)
        exx
        ld      de,(__csqrtf_imag)
        ld      hl,(__csqrtf_imag + 2)
        exx
        pop     ix
        ret
