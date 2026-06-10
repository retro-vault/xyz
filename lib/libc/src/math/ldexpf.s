        ;; ldexpf.s
        ;;
        ;; libc ldexpf implementation for the xcc Z80 libc.
        ;; Computes x * 2^n by adding n to the biased exponent.  Overflow
        ;; saturates to +/-Inf, underflow flushes to +/-0 (no denormals).
        ;; scalbnf is identical for FLT_RADIX == 2 and shares the entry.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module ldexpf
        .optsdcc -mz80 sdcccall(1)


        .globl  _ldexpf
        .globl  _scalbnf

        .area   _CODE

        ;; _ldexpf / _ldexp / _ldexpl / _scalbnf / _scalbn / _scalbnl
        ;; (double and long double are 32-bit on this target.)
        ;; inputs:  HL:DE = float x, 4(ix)..5(ix) = int n
        ;; outputs: HL:DE = x * 2^n
        ;; clobbers: AF, BC, IX
_scalbnf::
_ldexpf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ;; exp8 = ((H & 0x7F) << 1) | (L >> 7)
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        jr      z,ldexpf_exp_ok
        inc     a
ldexpf_exp_ok:
        or      a
        jr      z,ldexpf_return_x       ; exp == 0: +/-0 (denormal flushed)
        ;; newexp = exp8 + n  (16-bit signed, exp8 high byte is zero)
        add     a,4(ix)
        ld      c,a
        ld      a,#0
        adc     a,5(ix)
        ld      b,a                     ; BC = newexp
        ;; underflow when newexp <= 0
        bit     7,b
        jr      nz,ldexpf_underflow
        ld      a,b
        or      c
        jr      z,ldexpf_underflow
        ;; overflow when newexp >= 255
        ld      a,b                     ; high byte nonzero -> >= 256
        or      a
        jr      nz,ldexpf_overflow
        ld      a,c
        cp      #255
        jr      nc,ldexpf_overflow
        ;; write newexp (1..254) back into H:L
        srl     a                       ; A = newexp >> 1, carry = newexp & 1
        ld      b,a
        ld      a,h
        and     #0x80
        or      b
        ld      h,a
        ld      a,l
        and     #0x7f
        ld      l,a
        bit     0,c
        jr      z,ldexpf_return_x
        set     7,l
ldexpf_return_x:
        pop     ix
        ret
ldexpf_overflow:
        ld      a,h
        and     #0x80
        or      #0x7f
        ld      h,a                     ; sign | 0x7F
        ld      l,#0x80                 ; -> exponent 0xFF, mantissa 0 (Inf)
        ld      d,#0
        ld      e,#0
        pop     ix
        ret
ldexpf_underflow:
        ld      a,h
        and     #0x80                   ; preserve sign -> +/-0.0
        ld      h,a
        ld      l,#0
        ld      d,#0
        ld      e,#0
        pop     ix
        ret
