        ; sqrtf.s
        ;
        ; libc sqrtf for the xcc Z80 libc.  Newton-Raphson refinement built on
        ; the soft-float runtime (one body serves sqrtf/sqrt/sqrtl).  NaN, +/-0
        ; and +Inf pass through; a negative argument sets EDOM and returns NaN.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module sqrtf
        .optsdcc -mz80 sdcccall(1)
        .globl  _sqrtf
        .globl  _sqrt
        .globl  _sqrtl
        .globl  ___libc_fpclassifyf
        .globl  ___fsadd
        .globl  ___fsmul
        .globl  ___fsdiv
        .globl  __errno_value
        .area   _DATA
__sq_val:   .ds 4
__sq_guess: .ds 4
__sq_t:     .ds 4
        .area   _CODE
        ; HL:DE = value -> HL:DE = sqrt(value)
_sqrt::
_sqrtl::
_sqrtf::
        ld      (__sq_val),de
        ld      (__sq_val + 2),hl
        call    ___libc_fpclassifyf     ; DE = class (HL preserved)
        ld      a,e
        cp      #3
        jp      c,sqrt_ret_val          ; NaN/Inf/Zero -> return value
        ld      a,(__sq_val + 3)        ; sign (a3 bit7)
        bit     7,a
        jr      z,sqrt_pos
        ld      hl,#33                  ; EDOM
        ld      (__errno_value),hl
        ld      hl,#0x7fc0              ; NaN
        ld      de,#0x0000
        ret
sqrt_pos:
        ; e8 = (a3 & 0x7f) << 1 | (a2 >> 7)
        ld      a,(__sq_val + 3)
        and     #0x7f
        add     a,a
        ld      c,a
        ld      a,(__sq_val + 2)
        rlca
        and     #1
        or      c
        cp      #127
        jr      nc,sqrt_guess_val       ; value >= 1.0 -> guess = value
        ld      hl,#0x3f80              ; guess = 1.0
        ld      de,#0x0000
        jr      sqrt_store_guess
sqrt_guess_val:
        ld      de,(__sq_val)
        ld      hl,(__sq_val + 2)
sqrt_store_guess:
        ld      (__sq_guess),de
        ld      (__sq_guess + 2),hl
        ld      b,#8
sqrt_iter:
        push    bc
        ; t = value / guess
        ld      hl,(__sq_guess + 2)
        ld      bc,(__sq_guess)
        push    hl
        push    bc
        ld      de,(__sq_val)
        ld      hl,(__sq_val + 2)
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      (__sq_t),de
        ld      (__sq_t + 2),hl
        ; t = guess + t
        ld      hl,(__sq_t + 2)
        ld      bc,(__sq_t)
        push    hl
        push    bc
        ld      de,(__sq_guess)
        ld      hl,(__sq_guess + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__sq_t),de
        ld      (__sq_t + 2),hl
        ; next = 0.5 * t
        ld      hl,(__sq_t + 2)
        ld      bc,(__sq_t)
        push    hl
        push    bc
        ld      de,#0x0000              ; 0.5f
        ld      hl,#0x3f00
        call    ___fsmul
        pop     bc
        pop     bc
        ; compare next (DEHL) == guess
        ld      bc,(__sq_guess)
        ld      a,e
        cp      c
        jr      nz,sqrt_update
        ld      a,d
        cp      b
        jr      nz,sqrt_update
        ld      bc,(__sq_guess + 2)
        ld      a,l
        cp      c
        jr      nz,sqrt_update
        ld      a,h
        cp      b
        jr      nz,sqrt_update
        pop     bc                      ; converged
        jr      sqrt_done
sqrt_update:
        ld      (__sq_guess),de
        ld      (__sq_guess + 2),hl
        pop     bc
        dec     b
        jp      nz,sqrt_iter
sqrt_done:
        ld      hl,(__sq_guess + 2)
        ld      de,(__sq_guess)
        ret
sqrt_ret_val:
        ld      hl,(__sq_val + 2)
        ld      de,(__sq_val)
        ret
