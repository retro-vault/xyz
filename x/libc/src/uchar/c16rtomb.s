        ; c16rtomb.s
        ;
        ; libc c16rtomb() for the xcc Z80 libc.  A code unit in the unsigned
        ; byte range maps to one execution byte; anything larger is EILSEQ.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module c16rtomb
        .optsdcc -mz80 sdcccall(1)
        .globl  _c16rtomb
        .globl  __errno_value
        .area   _CODE

        ; _c16rtomb
        ; inputs:  HL = s, DE = c16, 4(ix) = ps
        ; outputs: DE = 1 (stored or s==NULL) or 0xFFFF (EILSEQ)
_c16rtomb::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; [s]
        push    de                      ; [c16]
        ld      l,4(ix)
        ld      h,5(ix)
        ld      a,h
        or      l
        jr      z,c16_nr
        ld      (hl),#0                 ; reset *ps
        inc     hl
        ld      (hl),#0
c16_nr:
        pop     de                      ; [c16]
        pop     hl                      ; [s]
        ld      a,h
        or      l
        jr      z,c16_ret1              ; s == NULL
        ld      a,d
        or      a
        jr      nz,c16_ilseq            ; value > 255
        ld      (hl),e
c16_ret1:
        ld      de,#1
        pop     ix
        ret
c16_ilseq:
        ld      hl,#84                  ; EILSEQ
        ld      (__errno_value),hl
        ld      de,#0xffff
        pop     ix
        ret
