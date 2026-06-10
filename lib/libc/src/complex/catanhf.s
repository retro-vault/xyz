        ;; catanhf.s
        ;;
        ;; libc catanhf() for the xcc Z80 libc.
        ;;
        ;; Principal-value inverse hyperbolic tangent via
        ;;   atanh(z) = 0.5 * (clog(1 + z) - clog(1 - z))
        ;; with component-wise subtraction on the complex results.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module catanhf
        .optsdcc -mz80 sdcccall(1)

        .globl  _catanhf
        .globl  _clogf
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsmul

        .area   _DATA
__catanhf_lp_re:
        .ds     4
__catanhf_lp_im:
        .ds     4
__catanhf_lm_re:
        .ds     4
__catanhf_lm_im:
        .ds     4
__catanhf_re:
        .ds     4
__catanhf_tmp:
        .ds     4

        .area   _CODE

_catanhf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; atanh(0) is exactly 0.
        ld      a,4(ix)
        or      5(ix)
        or      6(ix)
        or      7(ix)
        or      8(ix)
        or      9(ix)
        or      10(ix)
        or      11(ix)
        jr      nz,catanhf_nonzero
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        pop     ix
        ret

catanhf_nonzero:

        ;; clog(1 + z)
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__catanhf_tmp),de
        ld      (__catanhf_tmp + 2),hl
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      hl,(__catanhf_tmp + 2)
        push    hl
        ld      hl,(__catanhf_tmp)
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
        ld      (__catanhf_lp_re),de
        ld      (__catanhf_lp_re + 2),hl
        exx
        ld      (__catanhf_lp_im),de
        ld      (__catanhf_lp_im + 2),hl
        exx

        ;; clog(1 - z)
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        ld      (__catanhf_tmp),de
        ld      (__catanhf_tmp + 2),hl
        ld      a,11(ix)
        xor     #0x80                   ; imag(1-z) = -imag(z)
        ld      h,a
        ld      l,10(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      hl,(__catanhf_tmp + 2)
        push    hl
        ld      hl,(__catanhf_tmp)
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
        ld      (__catanhf_lm_re),de
        ld      (__catanhf_lm_re + 2),hl
        exx
        ld      (__catanhf_lm_im),de
        ld      (__catanhf_lm_im + 2),hl
        exx

        ;; 0.5 * (lp.re - lm.re)
        ld      hl,(__catanhf_lm_re + 2)
        push    hl
        ld      hl,(__catanhf_lm_re)
        push    hl
        ld      de,(__catanhf_lp_re)
        ld      hl,(__catanhf_lp_re + 2)
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
        ld      (__catanhf_re),de
        ld      (__catanhf_re + 2),hl

        ;; 0.5 * (lp.im - lm.im)
        ld      hl,(__catanhf_lm_im + 2)
        push    hl
        ld      hl,(__catanhf_lm_im)
        push    hl
        ld      de,(__catanhf_lp_im)
        ld      hl,(__catanhf_lp_im + 2)
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
        ld      (__catanhf_tmp),de
        ld      (__catanhf_tmp + 2),hl
        ld      de,(__catanhf_re)
        ld      hl,(__catanhf_re + 2)
        exx
        ld      de,(__catanhf_tmp)
        ld      hl,(__catanhf_tmp + 2)
        exx
        pop     ix
        ret
