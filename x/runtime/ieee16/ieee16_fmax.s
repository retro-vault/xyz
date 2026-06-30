        ; ieee16_fmax.s
        .module ieee16_fmax
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_fmax
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _fmaxf

        .area   _CODE
_ieee16_fmax::
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
        call    _fmaxf
        pop     bc
        pop     bc
        pop     bc
        call    ___fs2fh
        pop     ix
        ret
