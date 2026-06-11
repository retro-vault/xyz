        ;; modff.s
        ;;
        ;; libc modff implementation for the xcc Z80 libc.
        ;; Splits x into its integer part (stored through *iptr) and the
        ;; fractional part (the return value); both keep x's sign.  The integer
        ;; part is trunc(x); the fraction is x - trunc(x) via the runtime
        ;; soft-float subtract.  This file implements the float32 entry point;
        ;; double and long double use dedicated wrappers.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module modff
        .optsdcc -mz80 sdcccall(1)


        .globl  _modff
        .globl  _truncf
        .globl  ___fssub

        .area   _CODE

        ;; _modff / _modf / _modfl
        ;; inputs:  HL:DE = x, iptr (float *) on stack at 4(ix),5(ix)
        ;; outputs: HL:DE = fractional part; *iptr = integer part
        ;; clobbers: AF, BC, DE, HL, IX
_modff::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; save x high word at -2(ix),-1(ix)
        push    de                      ; save x low word at -4(ix),-3(ix)
        call    _truncf                 ; HL:DE = trunc(x)
        ;; store integer part through *iptr (a0,a1,a2,a3 = E,D,L,H)
        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = iptr
        ld      a,e
        ld      (bc),a
        inc     bc
        ld      a,d
        ld      (bc),a
        inc     bc
        ld      a,l
        ld      (bc),a
        inc     bc
        ld      a,h
        ld      (bc),a
        ;; frac = x - trunc(x): a = x, b = trunc(x) (HL:DE) on stack
        ld      c,e
        ld      b,d                     ; BC = trunc b0,b1
        push    hl                      ; trunc b2,b3
        push    bc                      ; trunc b0,b1
        ld      e,-4(ix)                ; reload x low word
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)                ; reload x high word
        call    ___fssub                ; DEHL = x - trunc(x)
        pop     bc
        pop     bc
        ;; the fractional part must carry x's sign; soft-float yields +0 for an
        ;; integer/zero x, so re-stamp the sign when the magnitude is zero.
        ld      a,h
        and     #0x7f
        or      l
        or      d
        or      e
        jr      nz,modf_done
        ld      a,-1(ix)                ; x's a3 (sign in bit7)
        and     #0x80
        ld      h,a
        ld      l,#0
        ld      d,#0
        ld      e,#0
modf_done:
        ld      sp,ix
        pop     ix
        ret
