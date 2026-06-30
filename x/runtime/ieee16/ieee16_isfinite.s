        ; ieee16_isfinite.s
        .module ieee16_isfinite
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_isfinite
        .globl  ___ieee16_classify_hl

        .area   _CODE
_ieee16_isfinite::
        call    ___ieee16_classify_hl
        ld      a,e
        cp      #2
        ld      de,#0
        jr      c,.done
        inc     de
.done:
        ret
