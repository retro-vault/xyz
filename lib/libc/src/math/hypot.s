        ;; hypot.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module hypot
        .optsdcc -mz80 sdcccall(1)

        .globl  _hypot
        .globl  _hypotl
        .globl  ___fs2db
        .globl  __db_load_arg0_fs
        .globl  __db_load_arg1_fs
        .globl  _hypotf

MD_XHI  .equ -10
MD_XLO  .equ -12
MD_YHI  .equ -6
MD_YLO  .equ -8

        .area   _CODE
_hypot::
_hypotl::
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
        call    _hypotf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

