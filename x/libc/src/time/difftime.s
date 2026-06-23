        ; difftime.s
        ;
        ; difftime() for the xcc Z80 libc, in assembly.  Computes the signed
        ; 32-bit second difference and folds it into the runtime int->float
        ; helper (double is a 32-bit float on this target).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module difftime
        .optsdcc -mz80 sdcccall(1)


        .globl  _difftime
        .globl  ___slong2fs

        .area   _CODE

        ; _difftime
        ; inputs:  DE:HL = end (DE=low16, HL=high16), 4(ix)..7(ix) = beginning
        ; outputs: HL:DE = (double)(end - beginning)   [double is 32-bit float]
        ; clobbers: AF, BC, DE, HL, IX
_difftime::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,e
        sub     a,4(ix)
        ld      e,a
        ld      a,d
        sbc     a,5(ix)
        ld      d,a
        ld      a,l
        sbc     a,6(ix)
        ld      l,a
        ld      a,h
        sbc     a,7(ix)
        ld      h,a                     ; DE:HL = end - beginning (int32)
        pop     ix
        jp      ___slong2fs             ; HL=high, DE=low -> (float) in HL:DE
