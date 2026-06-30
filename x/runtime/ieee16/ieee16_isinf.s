        ; ieee16_isinf.s
        .module ieee16_isinf
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_isinf
        .globl  ___ieee16_classify_hl

        .area   _CODE
_ieee16_isinf::
        call    ___ieee16_classify_hl
        ld      a,e
        cp      #1
        ld      de,#0
        ret     nz
        inc     de
        ret
