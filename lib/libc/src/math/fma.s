        ;; fma.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fma
        .optsdcc -mz80 sdcccall(1)

        .globl  _fma
        .globl  _fmal
        .globl  ___db2fs
        .globl  ___fs2db
        .globl  __db_load_arg0_fs
        .globl  __db_load_arg1_fs
        .globl  _fmaf

MD_XHI  .equ -10
MD_XLO  .equ -12
MD_YHI  .equ -6
MD_YLO  .equ -8
MD_ZHI  .equ -2
MD_ZLO  .equ -4

        .area   _CODE
__db_load_arg2_fs:
        ld      a,20(ix)
        ld      e,a
        ld      a,21(ix)
        ld      d,a
        ld      a,22(ix)
        ld      l,a
        ld      a,23(ix)
        ld      h,a
        exx
        ld      a,24(ix)
        ld      e,a
        ld      a,25(ix)
        ld      d,a
        ld      a,26(ix)
        ld      l,a
        ld      a,27(ix)
        ld      h,a
        exx
        jp      ___db2fs

        ;; The wrappers below are intentionally shallow:
        ;;   1. load stacked double argument(s),
        ;;   2. convert to float,
        ;;   3. reuse the proven float entry point,
        ;;   4. convert back to 64-bit double when the result is floating.
_fma::
_fmal::
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
        call    __db_load_arg1_fs
        ld      MD_YLO(ix),e
        ld      MD_YLO+1(ix),d
        ld      MD_YHI(ix),l
        ld      MD_YHI+1(ix),h
        call    __db_load_arg2_fs
        ld      MD_ZLO(ix),e
        ld      MD_ZLO+1(ix),d
        ld      MD_ZHI(ix),l
        ld      MD_ZHI+1(ix),h
        ld      l,MD_ZHI(ix)
        ld      h,MD_ZHI+1(ix)
        push    hl
        ld      l,MD_ZLO(ix)
        ld      h,MD_ZLO+1(ix)
        push    hl
        ld      l,MD_YHI(ix)
        ld      h,MD_YHI+1(ix)
        push    hl
        ld      l,MD_YLO(ix)
        ld      h,MD_YLO+1(ix)
        push    hl
        ld      e,MD_XLO(ix)
        ld      d,MD_XLO+1(ix)
        ld      l,MD_XHI(ix)
        ld      h,MD_XHI+1(ix)
        call    _fmaf
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

