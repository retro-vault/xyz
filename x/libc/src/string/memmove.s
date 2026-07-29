        ; memmove.s
        ;
        ; libc memmove implementation for the xcc Z80 libc.
        ; Picks LDIR for forward-safe copies and LDDR when the destination
        ; overlaps the source from above.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module memmove
        .optsdcc -mz80 sdcccall(1)


        .globl  _memmove

        .area   _CODE

        ; _memmove
        ; inputs:
        ;   HL         = destination
        ;   DE         = source
        ;   4(ix)..5(ix) = byte count
        ; outputs:
        ;   DE = original destination
        ; clobbers: AF, BC, HL, IX
_memmove::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; preserve original destination
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,memmove_done
        or      a                       ; clear carry before subtracting
        sbc     hl,de                   ; destination - source
        jr      z,memmove_done
        jr      c,memmove_forward
        pop     hl                      ; restore destination start
        push    hl
        add     hl,bc                   ; point at destination end
        dec     hl
        ex      de,hl                   ; DE = destination tail
        add     hl,bc                   ; HL = source end
        dec     hl
        lddr                            ; copy backwards across the overlap
        jr      memmove_done
memmove_forward:
        pop     hl                      ; restore destination start
        push    hl
        ex      de,hl                   ; HL = source, DE = destination
        ldir                            ; safe forward copy
memmove_done:
        pop     de
        pop     ix
        ret
