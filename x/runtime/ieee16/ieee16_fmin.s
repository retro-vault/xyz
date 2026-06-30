        ; ieee16_fmin.s
        .module ieee16_fmin
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_fmin
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _fminf

        .area   _CODE
_ieee16_fmin::
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
        call    _fminf
        pop     bc
        pop     bc
        pop     bc
        call    ___fs2fh
        pop     ix
        ret
