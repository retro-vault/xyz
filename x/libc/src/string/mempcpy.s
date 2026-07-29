        ; mempcpy.s
        ;
        ; libc mempcpy implementation for the xcc Z80 libc.
        ; Copies n bytes and returns dest + n (GNU extension); a straight
        ; LDIR is used since mempcpy, like memcpy, assumes no overlap.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module mempcpy
        .optsdcc -mz80 sdcccall(1)


        .globl  _mempcpy

        .area   _CODE

        ; _mempcpy
        ; inputs:  HL = destination, DE = source, 4(ix)..5(ix) = byte count
        ; outputs: DE = destination + count
        ; clobbers: AF, BC, HL, IX
_mempcpy::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        ex      de,hl                   ; HL = source, DE = destination
        ld      a,b
        or      c
        jr      z,mempcpy_done
        ldir                            ; copy BC bytes; DE ends at dest+count
mempcpy_done:
        pop     ix
        ret                             ; DE already = destination + count
