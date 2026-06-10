        ;; gammaf_core.s
        ;;
        ;; Shared single-precision lgammaf/tgammaf kernels for the xcc Z80
        ;; libc. Both paths shift the argument upward until y >= 8, then use
        ;; a short Stirling-log expansion:
        ;;   log Γ(y) ≈ (y - 1/2)log(y) - y + log(sqrt(2π))
        ;;             + 1/(12y) - 1/(360y^3) + 1/(1260y^5)
        ;;
        ;; lgammaf accumulates log(|factor|) during the shift and subtracts it
        ;; from the final Stirling estimate. tgammaf uses the same magnitude
        ;; path, then exponentiates and restores the sign implied by negative
        ;; recurrence factors.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module gammaf_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_lgammaf_core
        .globl  __libc_tgammaf_core
        .globl  ___libc_fpclassifyf
        .globl  _truncf
        .globl  _logf
        .globl  _expf
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsdiv
        .globl  ___fscmp

        .area   _DATA
__gcf_x:      .ds 4
__gcf_y:      .ds 4
__gcf_t:      .ds 4
__gcf_u:      .ds 4
__gcf_v:      .ds 4
__gcf_sign:   .ds 1

        .area   _CODE

__gcf_load_x:
        ld      de,(__gcf_x)
        ld      hl,(__gcf_x + 2)
        ret

__gcf_load_y:
        ld      de,(__gcf_y)
        ld      hl,(__gcf_y + 2)
        ret

__gcf_load_t:
        ld      de,(__gcf_t)
        ld      hl,(__gcf_t + 2)
        ret

__gcf_load_u:
        ld      de,(__gcf_u)
        ld      hl,(__gcf_u + 2)
        ret

__gcf_load_v:
        ld      de,(__gcf_v)
        ld      hl,(__gcf_v + 2)
        ret

__gcf_ret_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ret

__gcf_ret_pinf:
        ld      hl,#0x7f80
        ld      de,#0x0000
        ret

        ;; Detect non-positive integer poles:
        ;;   x < 0 and trunc(x) == x
__gcf_check_pole:
        ld      a,(__gcf_x + 3)
        and     #0x80
        ret     z
        call    __gcf_load_x
        call    _truncf
        ld      (__gcf_t),de
        ld      (__gcf_t + 2),hl
        ld      hl,(__gcf_x + 2)
        push    hl
        ld      hl,(__gcf_x)
        push    hl
        call    __gcf_load_t
        call    ___fscmp
        pop     bc
        pop     bc
        ld      a,d
        or      e
        ret

        ;; Return with HL:DE = log Γ(y) for the current y scratch.
__gcf_stirling_log_y:
        call    __gcf_load_y
        call    _logf
        ld      (__gcf_x),de            ; base = log(y)
        ld      (__gcf_x + 2),hl

        ;; t = y - 0.5
        ld      hl,#0x3f00
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_y
        call    ___fssub
        pop     bc
        pop     bc
        ld      (__gcf_t),de
        ld      (__gcf_t + 2),hl

        ;; base = (y - 0.5) * log(y)
        ld      hl,(__gcf_x + 2)
        push    hl
        ld      hl,(__gcf_x)
        push    hl
        call    __gcf_load_t
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__gcf_x),de
        ld      (__gcf_x + 2),hl

        ;; base -= y
        ld      hl,(__gcf_y + 2)
        push    hl
        ld      hl,(__gcf_y)
        push    hl
        call    __gcf_load_x
        call    ___fssub
        pop     bc
        pop     bc
        ld      (__gcf_x),de
        ld      (__gcf_x + 2),hl

        ;; base += log(sqrt(2π))
        ld      hl,#0x3f6b
        push    hl
        ld      hl,#0x3f8e
        push    hl
        call    __gcf_load_x
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__gcf_x),de
        ld      (__gcf_x + 2),hl

        ;; v = 1 / y
        ld      hl,(__gcf_y + 2)
        push    hl
        ld      hl,(__gcf_y)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      (__gcf_v),de
        ld      (__gcf_v + 2),hl

        ;; t = v * (1/12)
        ld      hl,#0x3daa
        push    hl
        ld      hl,#0xaaab
        push    hl
        call    __gcf_load_v
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__gcf_t),de
        ld      (__gcf_t + 2),hl

        ;; y = v^2
        ld      hl,(__gcf_v + 2)
        push    hl
        ld      hl,(__gcf_v)
        push    hl
        call    __gcf_load_v
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__gcf_y),de
        ld      (__gcf_y + 2),hl

        ;; u = v^3
        ld      hl,(__gcf_v + 2)
        push    hl
        ld      hl,(__gcf_v)
        push    hl
        call    __gcf_load_y
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__gcf_u),de
        ld      (__gcf_u + 2),hl

        ;; t += (-1/360) * v^3
        ld      hl,#0xbb36
        push    hl
        ld      hl,#0x0b61
        push    hl
        call    __gcf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__gcf_u),de
        ld      (__gcf_u + 2),hl
        ld      hl,(__gcf_u + 2)
        push    hl
        ld      hl,(__gcf_u)
        push    hl
        call    __gcf_load_t
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__gcf_t),de
        ld      (__gcf_t + 2),hl

        ;; u = v^5 = v^3 * v^2
        ld      hl,(__gcf_y + 2)
        push    hl
        ld      hl,(__gcf_y)
        push    hl
        call    __gcf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__gcf_u),de
        ld      (__gcf_u + 2),hl

        ;; t += (1/1260) * v^5
        ld      hl,#0x3a50
        push    hl
        ld      hl,#0x0d01
        push    hl
        call    __gcf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__gcf_u),de
        ld      (__gcf_u + 2),hl
        ld      hl,(__gcf_u + 2)
        push    hl
        ld      hl,(__gcf_u)
        push    hl
        call    __gcf_load_t
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__gcf_t),de
        ld      (__gcf_t + 2),hl

        ;; base + correction
        ld      hl,(__gcf_t + 2)
        push    hl
        ld      hl,(__gcf_t)
        push    hl
        call    __gcf_load_x
        call    ___fsadd
        pop     bc
        pop     bc
        ret

        ;; Build log(|Γ(x)|) into HL:DE and the tgamma sign flag into __gcf_sign.
__gcf_build_log_gamma:
        ld      (__gcf_y),de
        ld      (__gcf_y + 2),hl
        xor     a
        ld      (__gcf_sign),a
        ld      (__gcf_t),a
        ld      (__gcf_t + 1),a
        ld      (__gcf_t + 2),a
        ld      (__gcf_t + 3),a        ; logscale = 0

__gcf_loop_cmp:
        ld      hl,#0x4100             ; 8.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_y
        call    ___fscmp
        pop     bc
        pop     bc
        ld      a,d
        cp      #0xff
        jr      nz,__gcf_loop_done

        ;; sign ^= 1 for each negative recurrence factor.
        ld      a,(__gcf_y + 3)
        and     #0x80
        jr      z,__gcf_factor_abs
        ld      a,(__gcf_sign)
        xor     #1
        ld      (__gcf_sign),a

__gcf_factor_abs:
        call    __gcf_load_y
        res     7,h
        call    _logf
        ld      (__gcf_u),de
        ld      (__gcf_u + 2),hl

        ;; logscale += log(|y|)
        ld      hl,(__gcf_u + 2)
        push    hl
        ld      hl,(__gcf_u)
        push    hl
        call    __gcf_load_t
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__gcf_t),de
        ld      (__gcf_t + 2),hl

        ;; y += 1
        ld      hl,#0x3f80
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_y
        call    ___fsadd
        pop     bc
        pop     bc
        ld      (__gcf_y),de
        ld      (__gcf_y + 2),hl
        jr      __gcf_loop_cmp

__gcf_loop_done:
        call    __gcf_stirling_log_y
        ld      (__gcf_u),de
        ld      (__gcf_u + 2),hl

        ;; result = stirling_log - logscale
        ld      hl,(__gcf_t + 2)
        push    hl
        ld      hl,(__gcf_t)
        push    hl
        call    __gcf_load_u
        call    ___fssub
        pop     bc
        pop     bc
        ret

__libc_lgammaf_core::
        ld      (__gcf_x),de
        ld      (__gcf_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN
        jp      z,__gcf_ret_nan
        cp      #1                      ; +/-Inf
        jp      nz,__gcf_lgamma_not_inf
        ld      a,(__gcf_x + 3)
        and     #0x80
        jp      nz,__gcf_ret_nan
        jp      __gcf_ret_pinf
__gcf_lgamma_not_inf:
        cp      #2                      ; pole at +/-0
        jp      z,__gcf_ret_pinf
        call    __gcf_check_pole
        jp      z,__gcf_ret_pinf
        call    __gcf_load_x
        jp      __gcf_build_log_gamma

__libc_tgammaf_core::
        ld      (__gcf_x),de
        ld      (__gcf_x + 2),hl
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN
        jp      z,__gcf_ret_nan
        cp      #1                      ; +/-Inf
        jp      nz,__gcf_tgamma_not_inf
        ld      a,(__gcf_x + 3)
        and     #0x80
        jp      nz,__gcf_ret_nan
        jp      __gcf_ret_pinf
__gcf_tgamma_not_inf:
        cp      #2                      ; pole at +/-0
        jp      z,__gcf_ret_pinf
        call    __gcf_check_pole
        jp      z,__gcf_ret_pinf
        call    __gcf_load_x
        call    __gcf_build_log_gamma
        call    _expf
        ld      a,(__gcf_sign)
        or      a
        ret     z
        ld      a,h
        xor     #0x80
        ld      h,a
        ret
