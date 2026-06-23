        ;; sqrtf.s
        ;;
        ;; libc sqrtf for the xcc Z80 libc.  Newton-Raphson refinement built on
        ;; the soft-float runtime.  NaN, +/-0
        ;; and +Inf pass through; a negative argument sets EDOM and returns NaN.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module sqrtf
        .optsdcc -mz80 sdcccall(1)
        .globl  _sqrtf
        .globl  ___libc_fpclassifyf
        .globl  ___fsadd
        .globl  ___fsmul
        .globl  ___fsdiv
        .globl  __errno_value
        .area   _CODE

SQ_VAL_LO   .equ -12
SQ_VAL_HI   .equ -10
SQ_GUESS_LO .equ -8
SQ_GUESS_HI .equ -6
SQ_T_LO     .equ -4
SQ_T_HI     .equ -2

        ;; HL:DE = value -> HL:DE = sqrt(value)
_sqrtf::
        ld      b,h
        ld      c,l
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        ld      SQ_VAL_LO(ix),e
        ld      SQ_VAL_LO+1(ix),d
        ld      SQ_VAL_HI(ix),c
        ld      SQ_VAL_HI+1(ix),b
        ld      l,c
        ld      h,b
        call    ___libc_fpclassifyf     ; DE = class (HL preserved)
        ld      a,e
        cp      #3                      ; NaN/Inf/Zero -> return value
        jp      c,sqrt_ret_val
        ld      a,SQ_VAL_HI+1(ix)       ; sign (a3 bit7)
        bit     7,a
        jr      z,sqrt_pos
        ld      hl,#33                  ; EDOM
        ld      (__errno_value),hl
        ld      hl,#0x7fc0              ; NaN
        ld      de,#0x0000
        jp      sqrt_ret
sqrt_pos:
        ;; e8 = (a3 & 0x7f) << 1 | (a2 >> 7)
        ld      a,SQ_VAL_HI+1(ix)
        and     #0x7f
        add     a,a
        ld      c,a
        ld      a,SQ_VAL_HI(ix)
        rlca
        and     #1
        or      c
        cp      #127
        jr      nc,sqrt_guess_val       ; value >= 1.0 -> guess = value
        ld      hl,#0x3f80              ; guess = 1.0
        ld      de,#0x0000
        jr      sqrt_store_guess
sqrt_guess_val:
        ld      e,SQ_VAL_LO(ix)
        ld      d,SQ_VAL_LO+1(ix)
        ld      l,SQ_VAL_HI(ix)
        ld      h,SQ_VAL_HI+1(ix)
sqrt_store_guess:
        ld      SQ_GUESS_LO(ix),e
        ld      SQ_GUESS_LO+1(ix),d
        ld      SQ_GUESS_HI(ix),l
        ld      SQ_GUESS_HI+1(ix),h
        ld      b,#8
sqrt_iter:
        push    bc
        ;; t = value / guess
        ld      l,SQ_GUESS_HI(ix)
        ld      h,SQ_GUESS_HI+1(ix)
        push    hl
        ld      e,SQ_GUESS_LO(ix)
        ld      d,SQ_GUESS_LO+1(ix)
        push    de
        ld      e,SQ_VAL_LO(ix)
        ld      d,SQ_VAL_LO+1(ix)
        ld      l,SQ_VAL_HI(ix)
        ld      h,SQ_VAL_HI+1(ix)
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      SQ_T_LO(ix),e
        ld      SQ_T_LO+1(ix),d
        ld      SQ_T_HI(ix),l
        ld      SQ_T_HI+1(ix),h
        ;; t = guess + t
        ld      l,SQ_T_HI(ix)
        ld      h,SQ_T_HI+1(ix)
        push    hl
        ld      e,SQ_T_LO(ix)
        ld      d,SQ_T_LO+1(ix)
        push    de
        ld      e,SQ_GUESS_LO(ix)
        ld      d,SQ_GUESS_LO+1(ix)
        ld      l,SQ_GUESS_HI(ix)
        ld      h,SQ_GUESS_HI+1(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      SQ_T_LO(ix),e
        ld      SQ_T_LO+1(ix),d
        ld      SQ_T_HI(ix),l
        ld      SQ_T_HI+1(ix),h
        ;; next = 0.5 * t
        ld      l,SQ_T_HI(ix)
        ld      h,SQ_T_HI+1(ix)
        push    hl
        ld      e,SQ_T_LO(ix)
        ld      d,SQ_T_LO+1(ix)
        push    de
        ld      de,#0x0000              ; 0.5f
        ld      hl,#0x3f00
        call    ___fsmul
        pop     bc
        pop     bc
        ;; compare next (DEHL) == guess
        ld      a,e
        cp      SQ_GUESS_LO(ix)
        jr      nz,sqrt_update
        ld      a,d
        cp      SQ_GUESS_LO+1(ix)
        jr      nz,sqrt_update
        ld      a,l
        cp      SQ_GUESS_HI(ix)
        jr      nz,sqrt_update
        ld      a,h
        cp      SQ_GUESS_HI+1(ix)
        jr      nz,sqrt_update
        pop     bc                      ; converged
        jr      sqrt_done
sqrt_update:
        ld      SQ_GUESS_LO(ix),e
        ld      SQ_GUESS_LO+1(ix),d
        ld      SQ_GUESS_HI(ix),l
        ld      SQ_GUESS_HI+1(ix),h
        pop     bc
        dec     b
        jp      nz,sqrt_iter
sqrt_done:
        ld      l,SQ_GUESS_HI(ix)
        ld      h,SQ_GUESS_HI+1(ix)
        ld      e,SQ_GUESS_LO(ix)
        ld      d,SQ_GUESS_LO+1(ix)
        jr      sqrt_ret
sqrt_ret_val:
        ld      l,SQ_VAL_HI(ix)
        ld      h,SQ_VAL_HI+1(ix)
        ld      e,SQ_VAL_LO(ix)
        ld      d,SQ_VAL_LO+1(ix)
sqrt_ret:
        ld      sp,ix
        pop     ix
        ret
