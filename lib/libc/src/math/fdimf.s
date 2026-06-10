        ;; fdimf.s
        ;;
        ;; libc fdimf implementation for the xcc Z80 libc.
        ;; fdim(x,y) returns the positive difference x-y when x>y, otherwise +0.
        ;; Computed as d = x - y (runtime soft-float); a negative or zero result
        ;; collapses to +0. This file implements the float32 entry point;
        ;; double and long double use wrappers.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fdimf
        .optsdcc -mz80 sdcccall(1)


        .globl  _fdimf
        .globl  ___fssub

        .area   _DATA
__fdim_x:   .ds 4                       ; saved x while building the y operand

        .area   _CODE

        ;; HL:DE carries x. The second float, y, is stacked at 4(ix)..7(ix)
        ;; and must be rebuilt as the soft-float helper operand.
_fdimf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__fdim_x),de           ; save x low word
        ld      (__fdim_x + 2),hl       ; save x high word
        ;; Repack y in the order expected by ___fssub:
        ;; high word first, then low word.
        ld      a,7(ix)
        ld      h,a
        ld      a,6(ix)
        ld      l,a                     ; HL = y high word
        ld      a,5(ix)
        ld      b,a
        ld      a,4(ix)
        ld      c,a                     ; BC = y low word
        push    hl
        push    bc
        ld      de,(__fdim_x)           ; restore x low word
        ld      hl,(__fdim_x + 2)
        call    ___fssub                ; compute x - y
        pop     bc
        pop     bc                      ; drop y's temporary stack copy
        ;; Exact equality is already +0. Only negative results collapse.
        bit     7,h                     ; negative sign means x < y
        jr      nz,fdim_zero
        pop     ix
        ret                             ; positive difference survives unchanged
fdim_zero:
        ld      hl,#0
        ld      de,#0                   ; +0.0
        pop     ix
        ret
