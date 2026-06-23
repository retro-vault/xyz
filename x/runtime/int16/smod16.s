        ; Signed 16-bit modulus, SDCC register ABI.
        ; Sign of remainder matches sign of dividend (C11 semantics).
        ; Does NOT use __modsint (broken for opposite-sign operands).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module smod16
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __smod16
        .globl  __divuint

        ; __smod16
        ; inputs: HL = dividend (signed), DE = divisor (signed).
        ; outputs: DE = remainder (signed, sign = sign of dividend).
        ; clobbers: af, bc, de, hl, f.

__smod16:
        ld      a, h
        rla                             ; C = sign(dividend)
        push    af                      ; save sign on stack

        ; abs(dividend) in HL
        jr      nc, .d_pos
        xor     a
        sub     a, l
        ld      l, a
        sbc     a, a
        sub     a, h
        ld      h, a
.d_pos:
        ; HL = abs(dividend), DE = divisor
        ; swap to work on divisor: HL = divisor, DE = abs(dividend)
        ex      de, hl

        ; abs(divisor) in HL
        bit     7, h
        jr      z, .v_pos
        xor     a
        sub     a, l
        ld      l, a
        sbc     a, a
        sub     a, h
        ld      h, a
.v_pos:
        ; HL = abs(divisor), DE = abs(dividend)
        ex      de, hl                  ; HL = abs(dividend), DE = abs(divisor)

        call    __divuint               ; HL = unsigned remainder

        pop     af                      ; restore sign of dividend
        jr      nc, .done               ; positive: remainder already correct
        xor     a
        sub     a, l
        ld      l, a
        sbc     a, a
        sub     a, h
        ld      h, a
.done:
        ex      de, hl
        ret
