        ; Shared 16-bit multiply core.
        ; Adapted from retro-vault/libsdcc-z80 `src/int/mul.s`.
        ;
        ; ABI:

        .module mulint
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __mulint

        ; __mulint
        ; inputs: hl = multiplicand, de = multiplier
        ; outputs: de = product low 16
        ; clobbers: a, b, c, h, l, f

__mulint:
        ld      c, l
        ld      b, h

mul16_core:
        ld      a, b
        or      a, c
        jr      z, .ret_zero

        ld      a, d
        or      a, e
        jr      z, .ret_zero

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
        bit     0, c
        jr      z, .skip_add
        add     hl, de
.skip_add:
        sla     e
        rl      d
        srl     b
        rr      c
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
