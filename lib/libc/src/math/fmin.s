        ;; fmin.s
        ;; Split from legacymathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fmin
        .optsdcc -mz80 sdcccall(1)

        .globl  _fmin
        .globl  _fminl
        .globl  ___fs2db
        .globl  __lgd_load_arg0_fs
        .globl  __lgd_load_arg1_fs
        .globl  _fminf

LGD_XHI .equ -10
LGD_XLO .equ -12
LGD_YHI .equ -6
LGD_YLO .equ -8

        .area   _CODE
_fmin::
_fminl::
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
        call    __lgd_load_arg1_fs
        ld      LGD_YLO(ix),e
        ld      LGD_YLO+1(ix),d
        ld      LGD_YHI(ix),l
        ld      LGD_YHI+1(ix),h
        ld      l,LGD_YHI(ix)
        ld      h,LGD_YHI+1(ix)
        push    hl
        ld      e,LGD_YLO(ix)
        ld      d,LGD_YLO+1(ix)
        push    de
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    _fminf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

