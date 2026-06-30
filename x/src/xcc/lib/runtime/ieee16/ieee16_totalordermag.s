        ; ieee16_totalordermag.s
        .module ieee16_totalordermag
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_totalordermag
        .globl  ___ieee16_cmp_hl_de

        .area   _CODE
_ieee16_totalordermag::
        push    ix
        ld      ix,#0
        add     ix,sp
        res     7,h
        ld      e,4(ix)
        ld      d,5(ix)
        res     7,d
        call    ___ieee16_cmp_hl_de
        ld      de,#0
        cp      #0x01
        jr      z,.done
        inc     de
.done:
        pop     ix
        ret
