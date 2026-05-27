        ; shared float->u32 magnitude core for the runtime
        ;
        ; expects float already unpacked as:
        ;   E = a2 (high-word low byte)
        ;   H = a1
        ;   L = a0
        ;   C = unbiased exponent e (0..31)
        ;
        ; computes:
        ;   mag = (1.xxx mantissa) << or >> relative to bit 23
        ;
        ; outputs:
        ;   DE:HL = mag (unsigned 32-bit), DE high / HL low
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2026 tomaz stih

        .module fs2u32mag
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  __fs2u32mag

        ; __fs2u32mag
        ; inputs:  C=e(0..31), E=a2, H=a1, L=a0 from unpacked float
        ; outputs: DE:HL = unsigned 32-bit magnitude (DE high, HL low)
        ; clobbers: af, c, de, hl
__fs2u32mag:
        ; build 24-bit mantissa in DE:HL = 0 : M2 : M1 : M0
        ld      a,e
        and     #0x7F
        or      #0x80
        ld      e,a
        ld      d,#0x00

        ; shift based on e (C) relative to 23
        ld      a,c
        cp      #23
        ret     z
        jr      c, .shift_right

        ; left shift by (e - 23)
        sub     #23
.lsh_loop:
        sla     l
        rl      h
        rl      e
        rl      d
        dec     a
        jr      nz, .lsh_loop
        ret

.shift_right:
        ; right shift by (23 - e)
        ld      a,#23
        sub     c
.rsh_loop:
        srl     d
        rr      e
        rr      h
        rr      l
        dec     a
        jr      nz, .rsh_loop
        ret
