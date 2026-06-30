        ; ieee16_remainder.s
        .module ieee16_remainder
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_remainder
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _remainderf

        .area   _CODE
_ieee16_remainder::
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
        call    _remainderf
        pop     bc
        pop     bc
        pop     bc
        call    ___fs2fh
        pop     ix
        ret
