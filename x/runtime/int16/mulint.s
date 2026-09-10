        ; Shared 16-bit multiply core.
        ; Adapted from retro-vault/libsdcc-z80 `src/int/mul.s`.
        ;
        ; ABI:

        .module mulint
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __mul16
        .globl  __mulint

        ; __mulint
        ; inputs: hl = multiplicand, de = multiplier
        ; outputs: de = product low 16
        ; clobbers: a, b, c, h, l, f

__mul16:
__mulint:
        ld      c, l
        ld      b, h

mul16_core:
        ld      a, b
        or      a, c
        jr      z, .ret_zero

        ld      a, d
        or      a, e
        ret     z                       ; de already contains the zero result

        ld      a, c
        sub     a, e
        ld      a, b
        sbc     a, d
        jr      c, .no_swap

        ld      a, c
        ld      c, e
        ld      e, a
        ld      a, b
        ld      b, d
        ld      d, a

.no_swap:
        xor     a
        ld      h, a
        ld      l, a

.mul_loop:
        ; Shifting the multiplier exposes the same low bit in carry.
        ; Consume it directly, avoiding a separate BIT instruction.
        srl     b
        rr      c
        jr      nc, .skip_add
        add     hl, de
.skip_add:
        sla     e
        rl      d
        ld      a, b
        or      a, c
        jr      nz, .mul_loop

        ex      de, hl
        ret

.ret_zero:
        xor     a
        ld      d, a
        ld      e, a
        ret
