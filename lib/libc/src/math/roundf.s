        ;; roundf.s
        ;;
        ;; libc roundf implementation for the xcc Z80 libc.
        ;; Rounds to nearest, ties away from zero.  double / long double are
        ;; 32-bit on this target, so round / roundl share the implementation.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module roundf
        .optsdcc -mz80 sdcccall(1)


        .globl  _roundf
        .globl  _truncf
        .globl  __float_exp8
        .globl  __float_incmag

        .area   _CODE

        ;; _roundf / _round / _roundl
        ;; inputs:  HL:DE = float x
        ;; outputs: HL:DE = round(x)  (nearest, ties away from zero)
        ;; clobbers: AF, BC, IX
_roundf::
        call    __float_exp8            ; A = exp8 (HL:DE preserved)
        or      a
        jr      z,roundf_ret_x          ; x == 0 -> x
        cp      #126
        jr      c,roundf_toward_zero    ; |x| < 0.5 -> +/-0
        jr      z,roundf_away           ; 0.5 <= |x| < 1 -> +/-1
        ;; exp8 >= 127: e = exp8 - 127
        sub     #127
        cp      #23
        jr      nc,roundf_ret_x         ; e >= 23: integral, return x
        ;; p = 22 - e : index of the most-significant fractional bit
        ld      b,a
        ld      a,#22
        sub     b
        ld      b,a                     ; B = p (0..22)
        and     #7
        ld      c,a                     ; C = bit within byte
        ld      a,b
        srl     a
        srl     a
        srl     a                       ; A = byte index (0,1,2)
        or      a
        jr      z,roundf_byte_e
        dec     a
        jr      z,roundf_byte_d
        ld      a,l                     ; byte 2: mantissa high (mask exp bit)
        and     #0x7f
        jr      roundf_have_byte
roundf_byte_d:
        ld      a,d
        jr      roundf_have_byte
roundf_byte_e:
        ld      a,e
roundf_have_byte:
        ld      b,a                     ; B = source byte
        ld      a,#1
        inc     c
roundf_mask:
        dec     c
        jr      z,roundf_mask_done
        add     a,a
        jr      roundf_mask
roundf_mask_done:
        and     b                       ; test the fractional MSB
        jr      z,roundf_toward_zero    ; clear -> round toward zero
roundf_away:
        call    _truncf
        jp      __float_incmag
roundf_toward_zero:
        jp      _truncf
roundf_ret_x:
        ret
