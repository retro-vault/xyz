        ; ieee-754 double negate for the runtime
        ; flips sign bit (bit 63) of a double in DE:HL:DE':HL'
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module dbneg
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __dbneg

        ; __dbneg
        ; inputs:  DE:HL:DE':HL' = double a
        ;   DE  = bytes[1:0], HL = bytes[3:2], DE' = bytes[5:4], HL' = bytes[7:6]
        ; outputs: same registers with bit63 (H' bit7) toggled
        ; clobbers: af

__dbneg:
        exx
        ld      a, h
        xor     #0x80
        ld      h, a
        exx
        ret
