        ;; frexp.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module frexp
        .optsdcc -mz80 sdcccall(1)

        .globl  _frexp
        .globl  _frexpl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs

        .area   _CODE
_frexp::
_frexpl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        ld      c,12(ix)
        ld      b,13(ix)
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        jr      z,lgd_frexp_exp_ok
        inc     a
lgd_frexp_exp_ok:
        or      a
        jr      z,lgd_frexp_zero
        sub     #126
        ld      (bc),a
        inc     bc
        rla
        sbc     a,a
        ld      (bc),a
        ld      a,h
        and     #0x80
        or      #0x3f
        ld      h,a
        ld      a,l
        and     #0x7f
        ld      l,a
        jr      lgd_frexp_ret
lgd_frexp_zero:
        xor     a
        ld      (bc),a
        inc     bc
        ld      (bc),a
lgd_frexp_ret:
        call    ___fs2db
        pop     ix
        ret

