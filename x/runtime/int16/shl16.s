        ; shared 16-bit variable left-shift helper.
        ;
        ; used by `-Os` when a function contains repeated variable-count
        ; left shifts and outlining the loop is smaller than duplicating
        ; the inline shift ladder at every call site.
        ;
        ; ABI:
        ;   hl = input value
        ;   b  = shift count (low 8 bits)
        ; return:
        ;   hl = value << count
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module shl16
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __shl16

        ; __shl16
        ; inputs:  hl = input value, b = shift count
        ; outputs: hl = shifted result
        ; clobbers: af, b, h, l, f
__shl16:
        ld      a, b
        or      a, a
        ret     z

.loop:
        add     hl, hl
        djnz    .loop
        ret
