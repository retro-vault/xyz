        ; ieee16_isnan.s
        .module ieee16_isnan
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_isnan
        .globl  ___ieee16_classify_hl

        .area   _CODE
_ieee16_isnan::
        call    ___ieee16_classify_hl
        ld      a,d
        or      e
        ld      de,#0
        ret     nz
        inc     de
        ret
