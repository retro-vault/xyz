        ; iswctype.s
        ;
        ; libc iswctype() for the xcc Z80 libc.  Dispatches a __WCTYPE_* id to
        ; the matching isw* classifier through a jump table.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module iswctype
        .optsdcc -mz80 sdcccall(1)
        .globl  _iswctype
        .globl  _iswalnum, _iswalpha, _iswblank, _iswcntrl
        .globl  _iswdigit, _iswgraph, _iswlower, _iswprint
        .globl  _iswpunct, _iswspace, _iswupper, _iswxdigit
        .area   _CODE

        ; _iswctype
        ; inputs:  HL = wc, DE = desc (1..12)
        ; outputs: DE = boolean
_iswctype::
        push    hl                      ; [wc]
        ld      a,d
        or      a
        jr      nz,iswct_zero
        ld      a,e
        or      a
        jr      z,iswct_zero
        cp      #13
        jr      nc,iswct_zero
        dec     a
        add     a,a
        ld      c,a
        ld      b,#0
        ld      hl,#__iswct_tab
        add     hl,bc
        ld      e,(hl)
        inc     hl
        ld      d,(hl)                  ; DE = isw* address
        pop     hl                      ; HL = wc
        push    de
        ret                             ; tail-call isw*(wc)
iswct_zero:
        pop     hl
        ld      de,#0
        ret
__iswct_tab:
        .dw _iswalnum, _iswalpha, _iswblank, _iswcntrl
        .dw _iswdigit, _iswgraph, _iswlower, _iswprint
        .dw _iswpunct, _iswspace, _iswupper, _iswxdigit
