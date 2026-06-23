        ;; scalbln.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module scalbln
        .optsdcc -mz80 sdcccall(1)

        .globl  _scalbln
        .globl  _scalblnl
        .globl  ___fs2db
        .globl  __db_load_arg0_fs
        .globl  _scalblnf

MD_XHI  .equ -10
MD_XLO  .equ -12

        .area   _CODE
_scalbln::
_scalblnl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        ld      MD_XLO(ix),e
        ld      MD_XLO+1(ix),d
        ld      MD_XHI(ix),l
        ld      MD_XHI+1(ix),h
        ;; long n stays stacked as a 32-bit quantity for the float entry point.
        ld      a,15(ix)
        ld      h,a
        ld      a,14(ix)
        ld      l,a
        push    hl
        ld      a,13(ix)
        ld      h,a
        ld      a,12(ix)
        ld      l,a
        push    hl
        ld      e,MD_XLO(ix)
        ld      d,MD_XLO+1(ix)
        ld      l,MD_XHI(ix)
        ld      h,MD_XHI+1(ix)
        call    _scalblnf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

