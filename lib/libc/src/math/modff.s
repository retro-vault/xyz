        ; modff.s
        ;
        ; libc modff implementation for the xcc Z80 libc.
        ; Splits x into its integer part (stored through *iptr) and the
        ; fractional part (the return value); both keep x's sign.  The integer
        ; part is trunc(x); the fraction is x - trunc(x) via the runtime
        ; soft-float subtract.  One body serves modff / modf / modfl (all 32-bit
        ; floating types on this target).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module modff
        .optsdcc -mz80 sdcccall(1)


        .globl  _modff
        .globl  _modf
        .globl  _modfl
        .globl  _truncf
        .globl  ___fssub

        .area   _DATA
__modf_x:   .ds 4                       ; saved x while computing trunc(x)

        .area   _CODE

        ; _modff / _modf / _modfl
        ; inputs:  HL:DE = x, iptr (float *) on stack at 4(ix),5(ix)
        ; outputs: HL:DE = fractional part; *iptr = integer part
        ; clobbers: AF, BC, DE, HL, IX
_modf::
_modfl::
_modff::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__modf_x),de           ; save x (a0,a1)
        ld      (__modf_x + 2),hl       ;        (a2,a3)
        call    _truncf                 ; HL:DE = trunc(x)
        ; store integer part through *iptr (a0,a1,a2,a3 = E,D,L,H)
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
        ; frac = x - trunc(x): a = x, b = trunc(x) (HL:DE) on stack
        ld      c,e
        ld      b,d                     ; BC = trunc b0,b1
        push    hl                      ; trunc b2,b3
        push    bc                      ; trunc b0,b1
        ld      de,(__modf_x)           ; a = x
        ld      hl,(__modf_x + 2)
        call    ___fssub                ; DEHL = x - trunc(x)
        pop     bc
        pop     bc
        ; the fractional part must carry x's sign; soft-float yields +0 for an
        ; integer/zero x, so re-stamp the sign when the magnitude is zero.
        ld      a,h
        and     #0x7f
        or      l
        or      d
        or      e
        jr      nz,modf_done
        ld      a,(__modf_x + 3)        ; x's a3 (sign in bit7)
        and     #0x80
        ld      h,a
        ld      l,#0
        ld      d,#0
        ld      e,#0
modf_done:
        pop     ix
        ret
