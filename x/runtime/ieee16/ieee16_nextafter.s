        ; ieee16_nextafter.s
        .module ieee16_nextafter
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_nextafter
        .globl  ___ieee16_classify_hl
        .globl  ___ieee16_cmp_hl_de

        .area   _CODE
_ieee16_nextafter::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl
        call    ___ieee16_classify_hl
        ld      a,e
        cp      #0
        jr      z,.ret_saved_x
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,e
        ld      h,d
        call    ___ieee16_classify_hl
        ld      a,e
        cp      #0
        jr      z,.ret_y
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      e,4(ix)
        ld      d,5(ix)
        call    ___ieee16_cmp_hl_de
        or      a
        jr      z,.ret_y
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      a,h
        and     #0x7f
        or      l
        jr      nz,.nonzero
        ld      a,5(ix)
        and     #0x80
        ld      d,a
        ld      e,#0x01
        pop     bc
        pop     ix
        ret
.nonzero:
        bit     7,h
        jr      nz,.neg_x
        cp      #0xff
        jr      z,.inc_mag
        jr      .dec_mag
.neg_x:
        cp      #0xff
        jr      z,.dec_mag
.inc_mag:
        inc     l
        jr      nz,.ret_saved_x
        inc     h
        jr      .ret_saved_x
.dec_mag:
        dec     l
        jr      nz,.ret_saved_x
        dec     h
        jr      .ret_saved_x
.ret_y:
        ld      e,4(ix)
        ld      d,5(ix)
        pop     bc
        pop     ix
        ret
.ret_saved_x:
        ld      d,h
        ld      e,l
        pop     bc
        pop     ix
        ret
