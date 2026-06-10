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

        .area   _DATA
__cpowf_log_re:
        .ds     4
__cpowf_log_im:
        .ds     4
__cpowf_tmp0:
        .ds     4
__cpowf_tmp1:
        .ds     4
__cpowf_prod_re:
        .ds     4
__cpowf_prod_im:
        .ds     4

        .area   _CODE

_cpowf::
        push    ix
        ld      ix,#0
        add     ix,sp

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
        ld      (__cpowf_log_re),de
        ld      (__cpowf_log_re + 2),hl
        exx
        ld      (__cpowf_log_im),de
        ld      (__cpowf_log_im + 2),hl
        exx

        ;; tmp0 = log_re * b_re
        ld      l,14(ix)
        ld      h,15(ix)
        push    hl
        ld      l,12(ix)
        ld      h,13(ix)
        push    hl
        ld      de,(__cpowf_log_re)
        ld      hl,(__cpowf_log_re + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__cpowf_tmp0),de
        ld      (__cpowf_tmp0 + 2),hl

        ;; tmp1 = log_im * b_im
        ld      l,18(ix)
        ld      h,19(ix)
        push    hl
        ld      l,16(ix)
        ld      h,17(ix)
        push    hl
        ld      de,(__cpowf_log_im)
        ld      hl,(__cpowf_log_im + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__cpowf_tmp1),de
        ld      (__cpowf_tmp1 + 2),hl

        ;; prod_re = tmp0 - tmp1
        ld      hl,(__cpowf_tmp1 + 2)
        push    hl
        ld      hl,(__cpowf_tmp1)
        push    hl
        ld      de,(__cpowf_tmp0)
        ld      hl,(__cpowf_tmp0 + 2)
        call    ___fssub
        pop     bc
        pop     bc
        ld      (__cpowf_prod_re),de
        ld      (__cpowf_prod_re + 2),hl

        ;; tmp0 = log_re * b_im
        ld      l,18(ix)
        ld      h,19(ix)
        push    hl
        ld      l,16(ix)
        ld      h,17(ix)
        push    hl
        ld      de,(__cpowf_log_re)
        ld      hl,(__cpowf_log_re + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__cpowf_tmp0),de
        ld      (__cpowf_tmp0 + 2),hl

        ;; tmp1 = log_im * b_re
        ld      l,14(ix)
        ld      h,15(ix)
        push    hl
        ld      l,12(ix)
        ld      h,13(ix)
        push    hl
        ld      de,(__cpowf_log_im)
        ld      hl,(__cpowf_log_im + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__cpowf_tmp1),de
        ld      (__cpowf_tmp1 + 2),hl

        ;; prod_im = tmp0 + tmp1
        ld      hl,(__cpowf_tmp1 + 2)
        push    hl
        ld      hl,(__cpowf_tmp1)
        push    hl
        ld      de,(__cpowf_tmp0)
        ld      hl,(__cpowf_tmp0 + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__cpowf_prod_im),de
        ld      (__cpowf_prod_im + 2),hl

        ;; cexp(prod_re + i prod_im)
        ld      hl,(__cpowf_prod_im + 2)
        push    hl
        ld      hl,(__cpowf_prod_im)
        push    hl
        ld      hl,(__cpowf_prod_re + 2)
        push    hl
        ld      hl,(__cpowf_prod_re)
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
        pop     ix
        ret
