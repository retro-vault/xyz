        ; truncf.s
        ;
        ; libc truncf implementation for the xcc Z80 libc.
        ; Rounds a single-precision float toward zero by clearing the
        ; fractional mantissa bits, preserving the sign.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module truncf
        .optsdcc -mz80 sdcccall(1)


        .globl  _truncf
        .globl  _trunc
        .globl  _truncl

        .area   _CODE

        ; _truncf / _trunc / _truncl
        ; (double and long double are 32-bit on this target, so all three
        ;  share one implementation.)
        ; inputs:  HL:DE = float (H=a3 sign/exp, L=a2, D=a1, E=a0)
        ; outputs: HL:DE = trunc(x)
        ; clobbers: AF, BC
_trunc::
_truncl::
_truncf::
        ; exp8 = ((H & 0x7F) << 1) | (L >> 7)
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        jr      z,truncf_exp_ok
        inc     a
truncf_exp_ok:
        or      a
        jr      z,truncf_zero           ; exp == 0: zero / denormal -> +/-0
        sub     #127                    ; A = unbiased exponent e
        jr      c,truncf_zero           ; e < 0: |x| < 1 -> +/-0
        cp      #23
        jr      nc,truncf_return_x      ; e >= 23: no fractional bits, return x
        jr      truncf_mask
truncf_return_x:
        ret
truncf_mask:
        ; k = 23 - e fractional bits to clear
        ld      b,a
        ld      a,#23
        sub     b
        ld      b,a                     ; B = k (1..23)
        ; isolate L's exponent bit, keep mantissa in L[6:0]
        ld      a,l
        and     #0x80
        ld      c,a                     ; C = exp low bit
        ld      a,l
        and     #0x7f
        ld      l,a
        ; clear low k bits of the 24-bit field L:D:E (L is most significant)
        push    bc
truncf_shr:
        srl     l
        rr      d
        rr      e
        djnz    truncf_shr
        pop     bc
truncf_shl:
        sla     e
        rl      d
        rl      l
        djnz    truncf_shl
        ; restore L's exponent bit
        ld      a,l
        or      c
        ld      l,a
        ret
truncf_zero:
        ld      a,h
        and     #0x80                   ; preserve sign -> +/-0.0
        ld      h,a
        ld      l,#0
        ld      d,#0
        ld      e,#0
        ret
