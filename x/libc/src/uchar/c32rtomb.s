        ; c32rtomb.s
        ;
        ; libc c32rtomb() for the xcc Z80 libc.  A code unit in the unsigned
        ; byte range maps to one execution byte; anything larger is EILSEQ.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module c32rtomb
        .optsdcc -mz80 sdcccall(1)
        .globl  _c32rtomb
        .globl  __errno_value
        .area   _CODE

        ; _c32rtomb
        ; inputs:  HL = s, 4(ix)..7(ix) = c32, 8(ix) = ps
        ; outputs: DE = 1 (stored or s==NULL) or 0xFFFF (EILSEQ)
_c32rtomb::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; [s]
        ld      l,8(ix)
        ld      h,9(ix)
        ld      a,h
        or      l
        jr      z,c32_nr
        ld      (hl),#0                 ; reset *ps
        inc     hl
        ld      (hl),#0
c32_nr:
        pop     hl                      ; [s]
        ld      a,h
        or      l
        jr      z,c32_ret1              ; s == NULL
        ld      a,5(ix)
        or      6(ix)
        or      7(ix)
        jr      nz,c32_ilseq            ; value > 255
        ld      a,4(ix)
        ld      (hl),a
c32_ret1:
        ld      de,#1
        pop     ix
        ret
c32_ilseq:
        ld      hl,#84                  ; EILSEQ
        ld      (__errno_value),hl
        ld      de,#0xffff
        pop     ix
        ret
