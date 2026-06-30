        ; ieee16_totalorder.s
        .module ieee16_totalorder
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_totalorder
        .globl  ___ieee16_cmp_hl_de

        .area   _CODE
_ieee16_totalorder::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      e,4(ix)
        ld      d,5(ix)
        call    ___ieee16_cmp_hl_de
        ld      de,#0
        cp      #0x01
        jr      z,.done
        inc     de
.done:
        pop     ix
        ret
