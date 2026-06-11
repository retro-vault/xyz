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

        .area   _CODE

__gcf_load_x:
        ld      e,-21(ix)
        ld      d,-20(ix)
        ld      l,-19(ix)
        ld      h,-18(ix)
        ret

__gcf_load_y:
        ld      e,-17(ix)
        ld      d,-16(ix)
        ld      l,-15(ix)
        ld      h,-14(ix)
        ret

__gcf_load_t:
        ld      e,-13(ix)
        ld      d,-12(ix)
        ld      l,-11(ix)
        ld      h,-10(ix)
        ret

__gcf_load_u:
        ld      e,-9(ix)
        ld      d,-8(ix)
        ld      l,-7(ix)
        ld      h,-6(ix)
        ret

__gcf_load_v:
        ld      e,-5(ix)
        ld      d,-4(ix)
        ld      l,-3(ix)
        ld      h,-2(ix)
        ret

__gcf_cmp_x_half:
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_x
        call    ___fscmp
        ret

__gcf_cmp_x_1:
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_x
        call    ___fscmp
        ret

__gcf_cmp_x_2:
        ld      hl,#0x4000              ; 2.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_x
        call    ___fscmp
        ret

__gcf_cmp_x_3:
        ld      hl,#0x4040              ; 3.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_x
        call    ___fscmp
        ret

__gcf_cmp_x_4:
        ld      hl,#0x4080              ; 4.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_x
        call    ___fscmp
        ret

__gcf_cmp_x_5:
        ld      hl,#0x40a0              ; 5.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_x
        call    ___fscmp
        ret

__gcf_cmp_x_6:
        ld      hl,#0x40c0              ; 6.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_x
        call    ___fscmp
        ret

__gcf_cmp_x_7:
        ld      hl,#0x40e0              ; 7.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_x
        call    ___fscmp
        ret

__gcf_cmp_x_8:
        ld      hl,#0x4100              ; 8.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_x
        call    ___fscmp
        ret

__gcf_ret_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        jp      __gcf_finish

__gcf_ret_pinf:
        ld      hl,#0x7f80
        ld      de,#0x0000
        jp      __gcf_finish

__gcf_ret0:
        ld      hl,#0x0000
        ld      de,#0x0000
        jp      __gcf_finish

__gcf_ret_lnsqrtpi:
        ld      hl,#0x3f12
        ld      de,#0x8682
        jp      __gcf_finish

__gcf_ret_ln2:
        ld      hl,#0x3f31
        ld      de,#0x7218
        jp      __gcf_finish

__gcf_ret_ln6:
        ld      hl,#0x3fe5
        ld      de,#0x5860
        jp      __gcf_finish

__gcf_ret_ln24:
        ld      hl,#0x404b
        ld      de,#0x653c
        jp      __gcf_finish

__gcf_ret_ln120:
        ld      hl,#0x4099
        ld      de,#0x3322
        jp      __gcf_finish

__gcf_ret_ln720:
        ld      hl,#0x40d2
        ld      de,#0x893a
        jp      __gcf_finish

__gcf_ret_ln5040:
        ld      hl,#0x4108
        ld      de,#0x6710
        jp      __gcf_finish

__gcf_ret_1:
        ld      hl,#0x3f80
        ld      de,#0x0000
        jp      __gcf_finish

__gcf_ret_2:
        ld      hl,#0x4000
        ld      de,#0x0000
        jp      __gcf_finish

__gcf_ret_6:
        ld      hl,#0x40c0
        ld      de,#0x0000
        jp      __gcf_finish

__gcf_ret_24:
        ld      hl,#0x41c0
        ld      de,#0x0000
        jp      __gcf_finish

__gcf_ret_120:
        ld      hl,#0x42f0
        ld      de,#0x0000
        jp      __gcf_finish

__gcf_ret_720:
        ld      hl,#0x4434
        ld      de,#0x0000
        jp      __gcf_finish

__gcf_ret_5040:
        ld      hl,#0x459d
        ld      de,#0x8000
        jp      __gcf_finish

__gcf_ret_sqrtpi:
        ld      hl,#0x3fe2
        ld      de,#0xdfc5
        jp      __gcf_finish

        ;; Detect non-positive integer poles:
        ;;   x < 0 and trunc(x) == x
__gcf_check_pole:
        ld      a,-18(ix)
        and     #0x80
        jr      nz,__gcf_check_pole_neg
        ld      a,#1
        or      a
        ret
__gcf_check_pole_neg:
        call    __gcf_load_x
        call    _truncf
        ld      -13(ix),e
        ld      -12(ix),d
        ld      -11(ix),l
        ld      -10(ix),h
        ld      l,-19(ix)
        ld      h,-18(ix)
        push    hl
        ld      l,-21(ix)
        ld      h,-20(ix)
        push    hl
        call    __gcf_load_t
        call    ___fscmp
        ld      a,d
        or      e
        ret

        ;; Return with HL:DE = log Γ(y) for the current y scratch.
__gcf_stirling_log_y:
        call    __gcf_load_y
        call    _logf
        ld      -21(ix),e               ; base = log(y)
        ld      -20(ix),d
        ld      -19(ix),l
        ld      -18(ix),h

        ;; t = y - 0.5
        ld      hl,#0x3f00
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_y
        call    ___fssub
        pop     bc
        pop     bc
        ld      -13(ix),e
        ld      -12(ix),d
        ld      -11(ix),l
        ld      -10(ix),h

        ;; base = (y - 0.5) * log(y)
        ld      l,-19(ix)
        ld      h,-18(ix)
        push    hl
        ld      l,-21(ix)
        ld      h,-20(ix)
        push    hl
        call    __gcf_load_t
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -21(ix),e
        ld      -20(ix),d
        ld      -19(ix),l
        ld      -18(ix),h

        ;; base -= y
        ld      l,-15(ix)
        ld      h,-14(ix)
        push    hl
        ld      l,-17(ix)
        ld      h,-16(ix)
        push    hl
        call    __gcf_load_x
        call    ___fssub
        pop     bc
        pop     bc
        ld      -21(ix),e
        ld      -20(ix),d
        ld      -19(ix),l
        ld      -18(ix),h

        ;; base += log(sqrt(2π))
        ld      hl,#0x3f6b
        push    hl
        ld      hl,#0x3f8e
        push    hl
        call    __gcf_load_x
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -21(ix),e
        ld      -20(ix),d
        ld      -19(ix),l
        ld      -18(ix),h

        ;; v = 1 / y
        ld      l,-15(ix)
        ld      h,-14(ix)
        push    hl
        ld      l,-17(ix)
        ld      h,-16(ix)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fsdiv
        pop     bc
        pop     bc
        ld      -5(ix),e
        ld      -4(ix),d
        ld      -3(ix),l
        ld      -2(ix),h

        ;; t = v * (1/12)
        ld      hl,#0x3daa
        push    hl
        ld      hl,#0xaaab
        push    hl
        call    __gcf_load_v
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -13(ix),e
        ld      -12(ix),d
        ld      -11(ix),l
        ld      -10(ix),h

        ;; y = v^2
        ld      l,-3(ix)
        ld      h,-2(ix)
        push    hl
        ld      l,-5(ix)
        ld      h,-4(ix)
        push    hl
        call    __gcf_load_v
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -17(ix),e
        ld      -16(ix),d
        ld      -15(ix),l
        ld      -14(ix),h

        ;; u = v^3
        ld      l,-3(ix)
        ld      h,-2(ix)
        push    hl
        ld      l,-5(ix)
        ld      h,-4(ix)
        push    hl
        call    __gcf_load_y
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -9(ix),e
        ld      -8(ix),d
        ld      -7(ix),l
        ld      -6(ix),h

        ;; t += (-1/360) * v^3
        ld      hl,#0xbb36
        push    hl
        ld      hl,#0x0b61
        push    hl
        call    __gcf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -9(ix),e
        ld      -8(ix),d
        ld      -7(ix),l
        ld      -6(ix),h
        ld      l,-7(ix)
        ld      h,-6(ix)
        push    hl
        ld      l,-9(ix)
        ld      h,-8(ix)
        push    hl
        call    __gcf_load_t
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -13(ix),e
        ld      -12(ix),d
        ld      -11(ix),l
        ld      -10(ix),h

        ;; u = v^5 = v^3 * v^2
        ld      l,-15(ix)
        ld      h,-14(ix)
        push    hl
        ld      l,-17(ix)
        ld      h,-16(ix)
        push    hl
        call    __gcf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -9(ix),e
        ld      -8(ix),d
        ld      -7(ix),l
        ld      -6(ix),h

        ;; t += (1/1260) * v^5
        ld      hl,#0x3a50
        push    hl
        ld      hl,#0x0d01
        push    hl
        call    __gcf_load_u
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -9(ix),e
        ld      -8(ix),d
        ld      -7(ix),l
        ld      -6(ix),h
        ld      l,-7(ix)
        ld      h,-6(ix)
        push    hl
        ld      l,-9(ix)
        ld      h,-8(ix)
        push    hl
        call    __gcf_load_t
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -13(ix),e
        ld      -12(ix),d
        ld      -11(ix),l
        ld      -10(ix),h

        ;; base + correction
        ld      l,-11(ix)
        ld      h,-10(ix)
        push    hl
        ld      l,-13(ix)
        ld      h,-12(ix)
        push    hl
        call    __gcf_load_x
        call    ___fsadd
        pop     bc
        pop     bc
        ret

        ;; Build log(|Γ(x)|) into HL:DE and the tgamma sign flag into __gcf_sign.
__gcf_build_log_gamma:
        ld      -17(ix),e
        ld      -16(ix),d
        ld      -15(ix),l
        ld      -14(ix),h
        xor     a
        ld      -1(ix),a
        ld      -13(ix),a
        ld      -12(ix),a
        ld      -11(ix),a
        ld      -10(ix),a             ; logscale = 0

__gcf_loop_cmp:
        ld      hl,#0x4100             ; 8.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_y
        call    ___fscmp
        ld      a,d
        cp      #0xff
        jr      nz,__gcf_loop_done

        ;; sign ^= 1 for each negative recurrence factor.
        ld      a,-14(ix)
        and     #0x80
        jr      z,__gcf_factor_abs
        ld      a,-1(ix)
        xor     #1
        ld      -1(ix),a

__gcf_factor_abs:
        call    __gcf_load_y
        res     7,h
        call    _logf
        ld      -9(ix),e
        ld      -8(ix),d
        ld      -7(ix),l
        ld      -6(ix),h

        ;; logscale += log(|y|)
        ld      l,-7(ix)
        ld      h,-6(ix)
        push    hl
        ld      l,-9(ix)
        ld      h,-8(ix)
        push    hl
        call    __gcf_load_t
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -13(ix),e
        ld      -12(ix),d
        ld      -11(ix),l
        ld      -10(ix),h

        ;; y += 1
        ld      hl,#0x3f80
        push    hl
        ld      hl,#0x0000
        push    hl
        call    __gcf_load_y
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -17(ix),e
        ld      -16(ix),d
        ld      -15(ix),l
        ld      -14(ix),h
        jr      __gcf_loop_cmp

__gcf_loop_done:
        call    __gcf_stirling_log_y
        ld      -9(ix),e
        ld      -8(ix),d
        ld      -7(ix),l
        ld      -6(ix),h

        ;; result = stirling_log - logscale
        ld      l,-11(ix)
        ld      h,-10(ix)
        push    hl
        ld      l,-13(ix)
        ld      h,-12(ix)
        push    hl
        call    __gcf_load_u
        call    ___fssub
        pop     bc
        pop     bc
        ret

__libc_lgammaf_core::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l
        ld      b,h
        ld      hl,#-21
        add     hl,sp
        ld      sp,hl
        ld      -21(ix),e
        ld      -20(ix),d
        ld      -19(ix),c
        ld      -18(ix),b
        call    __gcf_load_x
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN
        jp      z,__gcf_ret_nan
        cp      #1                      ; +/-Inf
        jp      nz,__gcf_lgamma_not_inf
        ld      a,-18(ix)
        and     #0x80
        jp      nz,__gcf_ret_nan
        jp      __gcf_ret_pinf
__gcf_lgamma_not_inf:
        cp      #2                      ; pole at +/-0
        jp      z,__gcf_ret_pinf
        call    __gcf_check_pole
        jp      z,__gcf_ret_pinf
        call    __gcf_cmp_x_half
        ld      a,d
        or      e
        jp      z,__gcf_ret_lnsqrtpi
        call    __gcf_cmp_x_1
        ld      a,d
        or      e
        jp      z,__gcf_ret0
        call    __gcf_cmp_x_2
        ld      a,d
        or      e
        jp      z,__gcf_ret0
        call    __gcf_cmp_x_3
        ld      a,d
        or      e
        jp      z,__gcf_ret_ln2
        call    __gcf_cmp_x_4
        ld      a,d
        or      e
        jp      z,__gcf_ret_ln6
        call    __gcf_cmp_x_5
        ld      a,d
        or      e
        jp      z,__gcf_ret_ln24
        call    __gcf_cmp_x_6
        ld      a,d
        or      e
        jp      z,__gcf_ret_ln120
        call    __gcf_cmp_x_7
        ld      a,d
        or      e
        jp      z,__gcf_ret_ln720
        call    __gcf_cmp_x_8
        ld      a,d
        or      e
        jp      z,__gcf_ret_ln5040
        call    __gcf_load_x
        call    __gcf_build_log_gamma
        jp      __gcf_finish

__libc_tgammaf_core::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l
        ld      b,h
        ld      hl,#-21
        add     hl,sp
        ld      sp,hl
        ld      -21(ix),e
        ld      -20(ix),d
        ld      -19(ix),c
        ld      -18(ix),b
        call    __gcf_load_x
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN
        jp      z,__gcf_ret_nan
        cp      #1                      ; +/-Inf
        jp      nz,__gcf_tgamma_not_inf
        ld      a,-18(ix)
        and     #0x80
        jp      nz,__gcf_ret_nan
        jp      __gcf_ret_pinf
__gcf_tgamma_not_inf:
        cp      #2                      ; pole at +/-0
        jp      z,__gcf_ret_pinf
        call    __gcf_check_pole
        jp      z,__gcf_ret_pinf
        call    __gcf_cmp_x_half
        ld      a,d
        or      e
        jp      z,__gcf_ret_sqrtpi
        call    __gcf_cmp_x_1
        ld      a,d
        or      e
        jp      z,__gcf_ret_1
        call    __gcf_cmp_x_2
        ld      a,d
        or      e
        jp      z,__gcf_ret_1
        call    __gcf_cmp_x_3
        ld      a,d
        or      e
        jp      z,__gcf_ret_2
        call    __gcf_cmp_x_4
        ld      a,d
        or      e
        jp      z,__gcf_ret_6
        call    __gcf_cmp_x_5
        ld      a,d
        or      e
        jp      z,__gcf_ret_24
        call    __gcf_cmp_x_6
        ld      a,d
        or      e
        jp      z,__gcf_ret_120
        call    __gcf_cmp_x_7
        ld      a,d
        or      e
        jp      z,__gcf_ret_720
        call    __gcf_cmp_x_8
        ld      a,d
        or      e
        jp      z,__gcf_ret_5040
        call    __gcf_load_x
        call    __gcf_build_log_gamma
        call    _expf
        ld      a,-1(ix)
        or      a
        jr      z,__gcf_finish
        ld      a,h
        xor     #0x80
        ld      h,a
__gcf_finish:
        ld      sp,ix
        pop     ix
        ret
