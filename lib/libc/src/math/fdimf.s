        ; fdimf.s
        ;
        ; libc fdimf implementation for the xcc Z80 libc.
        ; fdim(x,y) returns the positive difference x-y when x>y, otherwise +0.
        ; Computed as d = x - y (runtime soft-float); a negative or zero result
        ; collapses to +0.  (float, double and long double are all 32-bit here,
        ; so one body serves fdimf / fdim / fdiml.)
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fdimf
        .optsdcc -mz80 sdcccall(1)


        .globl  _fdimf
        .globl  _fdim
        .globl  _fdiml
        .globl  ___fssub

        .area   _DATA
__fdim_x:   .ds 4                       ; saved x while building the y operand

        .area   _CODE

        ; _fdimf / _fdim / _fdiml
        ; inputs:  HL:DE = x (H=a3 sign/exp..E=a0), y on stack (4(ix)..7(ix))
        ; outputs: HL:DE = fdim(x, y)
        ; clobbers: AF, BC, DE, HL, IX
_fdim::
_fdiml::
_fdimf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__fdim_x),de           ; a0,a1
        ld      (__fdim_x + 2),hl       ; a2,a3
        ; build y as the soft-float stack operand: push hl(b2,b3), push bc(b0,b1)
        ld      a,7(ix)
        ld      h,a
        ld      a,6(ix)
        ld      l,a                     ; HL = y2,y3
        ld      a,5(ix)
        ld      b,a
        ld      a,4(ix)
        ld      c,a                     ; BC = y0,y1
        push    hl
        push    bc
        ld      de,(__fdim_x)           ; a = x in DEHL
        ld      hl,(__fdim_x + 2)
        call    ___fssub                ; DEHL = x - y
        pop     bc
        pop     bc                      ; drop the 4-byte operand
        bit     7,h                     ; result negative?  (x < y)
        jr      nz,fdim_zero
        pop     ix
        ret                             ; x >= y: return x - y (>= +0)
fdim_zero:
        ld      hl,#0
        ld      de,#0                   ; +0.0
        pop     ix
        ret
