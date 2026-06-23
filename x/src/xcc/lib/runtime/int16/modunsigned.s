        ; Shared 16-bit unsigned modulus helper.
        ; Adapted from retro-vault/libsdcc-z80 `src/int/modunsigned.s`.
        ;
        ; ABI:

        .module modunsigned
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __mod16
        .globl  __moduint
        .globl  __divuint

        ; __moduint
        ; inputs: hl = dividend, de = divisor
        ; outputs: de = remainder
        ; clobbers: a, b, c, d, e, h, l, f

__mod16:
__moduint:
        call    __divuint       ; HL = remainder, DE = quotient
        ex      de, hl
        ret
