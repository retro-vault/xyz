        ; ieee16_hypot.s
        .module ieee16_hypot
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_hypot
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _hypotf

        .area   _CODE
_ieee16_hypot::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        call    ___fh2fs
        push    hl
        push    de
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    ___fh2fs
        call    _hypotf
        pop     bc
        pop     bc
        pop     bc
        call    ___fs2fh
        pop     ix
        ret
