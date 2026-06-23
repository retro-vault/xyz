        ; signed 16-bit division core, with remainder sign fixup helper
        ; builds absolute values, divides via __divuint, then fixes signs
        ;
        ; loosely based on code from sdcc project
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2000-2021 michael hope, philipp klaus krause
        ; copyright (c) 2026 tomaz stih

        .module divsigned
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE                   ; code segment

        .globl  __divsint_rrx_s
        .globl  __divsint_rrf_s
        .globl  __divsint               ; export symbols
        .globl  __sdiv16
        .globl  __get_remainder
        .globl  __divuint

        ; __sdiv16 / __divsint
        ; inputs:  hl = dividend (signed 16-bit), de = divisor (signed
        ; 16-bit)
        ; outputs: de = quotient (signed 16-bit), hl = remainder (signed
        ; 16-bit)
        ; clobbers: a, b, d, e, h, l, f
        ; notes: take abs values, do unsigned divide, then fix signs
__divsint_rrx_s::
__divsint_rrf_s::
__sdiv16:
__divsint:
        ld      a, h
        xor     a, d
        rla
        ld      a, h
        push    af

        ; take absolute value of dividend
        rla                             ; test sign(dividend)
        jr      nc, .chkde
        sub     a, a                    ; a = 0
        sub     a, l                    ; a = -low
        ld      l, a                    ; l = -low
        sbc     a, a                    ; a = ff if borrow
        sub     a, h                    ; a = -high - borrow
        ld      h, a                    ; h = -high

        ; take absolute value of divisor
.chkde:
        bit     7, d                    ; test sign(divisor)
        jr      z, .dodiv
        sub     a, a                    ; a = 0
        sub     a, e                    ; a = -low
        ld      e, a                    ; e = -low
        sbc     a, a                    ; a = ff if borrow
        sub     a, d                    ; a = -high - borrow
        ld      d, a                    ; d = -high

        ; divide absolute values (unsigned core)
.dodiv:
        call    __divuint

.fix_quotient:
        ; negate quotient if it should be negative
        pop     af
        jr      nc, .ret_with_dividend_af
        push    af                      ; __get_remainder needs original AF
        call    .neg_de
        pop     af
        ; fall through with AF restored for __get_remainder callers
.ret_with_dividend_af:
        ret

        ; __get_remainder
        ; inputs:  carry encodes sign(dividend) from prior rla
        ; outputs: hl = remainder (signed 16-bit, sign matches dividend)
        ; clobbers: a, d, e, f; de used as temp
__get_remainder::
        rla
        ex      de, hl
        jr      nc, .done               ; if positive, done
        call    .neg_de
.done:
        ex      de, hl                  ; return remainder in hl
        ret

.neg_de:
        sub     a, a                    ; a = 0
        sub     a, e                    ; a = -low
        ld      e, a                    ; e = low negated
        sbc     a, a                    ; a = ff if borrow
        sub     a, d                    ; a = -high - borrow
        ld      d, a                    ; d = high negated
        ret
