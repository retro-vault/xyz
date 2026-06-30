        ; ieee16_setpayloadsig.s
        .module ieee16_setpayloadsig
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_setpayloadsig
        .globl  ___ieee16_store_half_ptr_de

        .area   _CODE
_ieee16_setpayloadsig::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl
        ld      e,4(ix)
        ld      d,5(ix)
        ld      a,d
        and     #0x03
        ld      d,a
        ld      a,d
        or      e
        jr      nz,.have_payload
        ld      de,#1
        pop     hl
        pop     ix
        ret
.have_payload:
        ld      a,d
        or      #0x7c
        and     #0xfd
        ld      d,a
        pop     hl
        call    ___ieee16_store_half_ptr_de
        ld      de,#0
        pop     ix
        ret
