        ; ieee16_fminimum.s
        .module ieee16_fminimum
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_fminimum
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _fminimumf

        .area   _CODE
_ieee16_fminimum::
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
        call    _fminimumf
        pop     bc
        pop     bc
        pop     bc
        call    ___fs2fh
        pop     ix
        ret
