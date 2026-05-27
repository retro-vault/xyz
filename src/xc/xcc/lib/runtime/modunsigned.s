        ; Shared 16-bit unsigned modulus helper.
        ; Adapted from retro-vault/libsdcc-z80 `src/int/modunsigned.s`.
        ;
        ; ABI:

        .module modunsigned
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __moduint
        .globl  __divuint

        ; __moduint
        ; inputs: hl = dividend, de = divisor
        ; outputs: hl = remainder
        ; clobbers: a, b, c, d, e, h, l, f

__moduint:
        call    __divuint
        ex      de, hl
        ret
