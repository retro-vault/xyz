        ;; cacoshf.s
        ;;
        ;; libc cacoshf() for the xcc Z80 libc.
        ;;
        ;; Principal-value inverse hyperbolic cosine via
        ;;   acosh(z) = clog(z + csqrt(z + 1) * csqrt(z - 1))
        ;; using the existing square-root/log kernels plus explicit complex
        ;; multiplication.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cacoshf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cacoshf
        .globl  _csqrtf
        .globl  _clogf
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsmul

        .area   _DATA
__cacoshf_a_re:
        .ds     4
__cacoshf_a_im:
        .ds     4
__cacoshf_b_re:
        .ds     4
__cacoshf_b_im:
        .ds     4
__cacoshf_prod_re:
        .ds     4
__cacoshf_prod_im:
        .ds     4
__cacoshf_tmp:
        .ds     4

        .area   _CODE

_cacoshf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; acosh(1) is exactly 0.
        ld      a,4(ix)
        or      5(ix)
        jr      nz,cacoshf_generic
        ld      a,6(ix)
        cp      #0x80
        jr      nz,cacoshf_generic
        ld      a,7(ix)
        cp      #0x3f
        jr      nz,cacoshf_generic
        ld      a,8(ix)
        or      9(ix)
        or      10(ix)
        or      11(ix)
        jr      nz,cacoshf_generic
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        pop     ix
        ret

cacoshf_generic:

        ;; a = csqrt(z + 1)
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
        ld      (__cacoshf_tmp),de
        ld      (__cacoshf_tmp + 2),hl
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      hl,(__cacoshf_tmp + 2)
        push    hl
        ld      hl,(__cacoshf_tmp)
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
        ld      (__cacoshf_a_re),de
        ld      (__cacoshf_a_re + 2),hl
        exx
        ld      (__cacoshf_a_im),de
        ld      (__cacoshf_a_im + 2),hl
        exx

        ;; b = csqrt(z - 1)
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      (__cacoshf_tmp),de
        ld      (__cacoshf_tmp + 2),hl
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      hl,(__cacoshf_tmp + 2)
        push    hl
        ld      hl,(__cacoshf_tmp)
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
        ld      (__cacoshf_b_re),de
        ld      (__cacoshf_b_re + 2),hl
        exx
        ld      (__cacoshf_b_im),de
        ld      (__cacoshf_b_im + 2),hl
        exx

        ;; prod.re = a.re*b.re - a.im*b.im
        ld      hl,(__cacoshf_b_re + 2)
        push    hl
        ld      hl,(__cacoshf_b_re)
        push    hl
        ld      de,(__cacoshf_a_re)
        ld      hl,(__cacoshf_a_re + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__cacoshf_tmp),de
        ld      (__cacoshf_tmp + 2),hl
        ld      hl,(__cacoshf_b_im + 2)
        push    hl
        ld      hl,(__cacoshf_b_im)
        push    hl
        ld      de,(__cacoshf_a_im)
        ld      hl,(__cacoshf_a_im + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      de,(__cacoshf_tmp)
        ld      hl,(__cacoshf_tmp + 2)
        call    ___fssub
        pop     bc
        pop     bc
        ld      (__cacoshf_prod_re),de
        ld      (__cacoshf_prod_re + 2),hl

        ;; prod.im = a.re*b.im + a.im*b.re
        ld      hl,(__cacoshf_b_im + 2)
        push    hl
        ld      hl,(__cacoshf_b_im)
        push    hl
        ld      de,(__cacoshf_a_re)
        ld      hl,(__cacoshf_a_re + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__cacoshf_tmp),de
        ld      (__cacoshf_tmp + 2),hl
        ld      hl,(__cacoshf_b_re + 2)
        push    hl
        ld      hl,(__cacoshf_b_re)
        push    hl
        ld      de,(__cacoshf_a_im)
        ld      hl,(__cacoshf_a_im + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      de,(__cacoshf_tmp)
        ld      hl,(__cacoshf_tmp + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__cacoshf_prod_im),de
        ld      (__cacoshf_prod_im + 2),hl

        ;; clog(z + prod)
        ld      hl,(__cacoshf_prod_re + 2)
        push    hl
        ld      hl,(__cacoshf_prod_re)
        push    hl
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__cacoshf_tmp),de
        ld      (__cacoshf_tmp + 2),hl
        ld      hl,(__cacoshf_prod_im + 2)
        push    hl
        ld      hl,(__cacoshf_prod_im)
        push    hl
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__cacoshf_prod_im),de
        ld      (__cacoshf_prod_im + 2),hl
        ld      hl,(__cacoshf_prod_im + 2)
        push    hl
        ld      hl,(__cacoshf_prod_im)
        push    hl
        ld      hl,(__cacoshf_tmp + 2)
        push    hl
        ld      hl,(__cacoshf_tmp)
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
