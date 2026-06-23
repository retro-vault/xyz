        ; shared 16-bit variable logical right-shift helper.
        ;
        ; used by `-Os` when a function contains repeated variable-count
        ; unsigned right shifts and outlining the loop is smaller than
        ; duplicating the inline shift ladder at every call site.
        ;
        ; ABI:
        ;   hl = input value
        ;   b  = shift count (low 8 bits)
        ; return:
        ;   hl = value >> count (logical)
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module shr16u
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __shr16u

        ; __shr16u
        ; inputs:  hl = input value, b = shift count
        ; outputs: hl = shifted result
        ; clobbers: af, b, h, l, f
__shr16u:
        ld      a, b
        or      a, a
        ret     z

.loop:
        srl     h
        rr      l
        djnz    .loop
        ret
