        ; shared float->u16 magnitude core for the runtime
        ;
        ; expects float already unpacked as:
        ;   C = a2 (high-word low byte)
        ;   H = a1
        ;   L = a0
        ;   A = unbiased exponent e (0..15)
        ;
        ; computes:
        ;   mag = (1.xxx mantissa) >> (23 - e)
        ;
        ; outputs:
        ;   DE = mag (unsigned 16-bit)
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module fs2u16mag
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __fs2u16mag

        ; __fs2u16mag
        ; inputs:  A=e(0..15), C=a2, H=a1, L=a0 from unpacked float
        ; outputs: DE = unsigned 16-bit magnitude
        ; clobbers: af, bc, de
__fs2u16mag:
        ld      e,a

        ; build mantissa D:B:C = 1.xxx (24-bit)
        ld      d,c
        ld      b,h
        ld      c,l
        ld      a,d
        and     #0x7F
        or      #0x80
        ld      d,a

        ; shift right by (23 - e)
        ld      a,#23
        sub     a,e
.rsh_loop:
        srl     d
        rr      b
        rr      c
        dec     a
        jr      nz,.rsh_loop

        ; result = B:C
        ld      d,b
        ld      e,c
        ret
