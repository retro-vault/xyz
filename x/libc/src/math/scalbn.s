        ;; scalbn.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module scalbn
        .optsdcc -mz80 sdcccall(1)

        .globl  _scalbn
        .globl  _scalbnl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  _scalbnf

LGD_XHI .equ -10
LGD_XLO .equ -12

        .area   _CODE
_scalbn::
_scalbnl::
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
        ld      a,13(ix)
        ld      h,a
        ld      a,12(ix)
        ld      l,a
        push    hl
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    _scalbnf
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

