        ;; frexpf.s
        ;;
        ;; libc frexpf implementation for the xcc Z80 libc.
        ;; Splits x into a normalized fraction m in [0.5,1) and an exponent e
        ;; such that x == m * 2^e, storing e through the supplied pointer.
        ;; Denormals are flushed to zero.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module frexpf
        .optsdcc -mz80 sdcccall(1)


        .globl  _frexpf
        .area   _CODE

        ;; _frexpf / _frexp / _frexpl  (32-bit float types on this target)
        ;; inputs:  HL:DE = float x, 4(ix)..5(ix) = int *exp
        ;; outputs: HL:DE = fraction m in [0.5,1) (or 0); *exp = e
        ;; clobbers: AF, BC, IX
_frexpf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = exp pointer
        ;; exp8 = ((H & 0x7F) << 1) | (L >> 7)
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        jr      z,frexpf_exp_ok
        inc     a
frexpf_exp_ok:
        or      a
        jr      z,frexpf_zero           ; 0 / denormal -> m = x, e = 0
        ;; e = exp8 - 126  (so the fraction lands in [0.5, 1))
        sub     #126
        ld      (bc),a                  ; *exp low = e
        inc     bc
        rla
        sbc     a,a                     ; sign-extend e
        ld      (bc),a                  ; *exp high
        ;; force the fraction's exponent to 126 (biased)
        ld      a,h
        and     #0x80
        or      #0x3f                   ; 126 >> 1 = 0x3F
        ld      h,a
        ld      a,l
        and     #0x7f                   ; 126 & 1 == 0 -> clear bit 7
        ld      l,a
        pop     ix
        ret
frexpf_zero:
        xor     a
        ld      (bc),a                  ; *exp = 0
        inc     bc
        ld      (bc),a
        ;; m = x unchanged (preserves +/-0)
        pop     ix
        ret
