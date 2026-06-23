        ;; modf.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module modf
        .optsdcc -mz80 sdcccall(1)

        .globl  _modf
        .globl  _modfl
        .globl  ___fs2db
        .globl  ___fssub
        .globl  __lgd_load_arg0_fs
        .globl  _truncf

LGD_IHI .equ -2
LGD_ILO .equ -4
LGD_XHI .equ -10
LGD_XLO .equ -12

        .area   _CODE
__lgd_store_result_at_bc:
        ld      a,e
        ld      (bc),a
        inc     bc
        ld      a,d
        ld      (bc),a
        inc     bc
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
        inc     bc
        push    bc
        exx
        pop     bc
        ld      a,e
        ld      (bc),a
        inc     bc
        ld      a,d
        ld      (bc),a
        inc     bc
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
        exx
        ret

_modf::
_modfl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __lgd_load_arg0_fs
        ld      LGD_XLO(ix),e
        ld      LGD_XLO+1(ix),d
        ld      LGD_XHI(ix),l
        ld      LGD_XHI+1(ix),h
        call    _truncf
        ld      LGD_ILO(ix),e
        ld      LGD_ILO+1(ix),d
        ld      LGD_IHI(ix),l
        ld      LGD_IHI+1(ix),h
        call    ___fs2db
        ld      c,12(ix)
        ld      b,13(ix)
        call    __lgd_store_result_at_bc
        ld      l,LGD_IHI(ix)
        ld      h,LGD_IHI+1(ix)
        push    hl
        ld      e,LGD_ILO(ix)
        ld      d,LGD_ILO+1(ix)
        push    de
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    ___fssub
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

