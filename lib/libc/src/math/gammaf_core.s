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
        .globl  ___libc_fpclassifyf
        .globl  __gcf_build_log_gamma
        .globl  __gcf_check_pole
        .globl  __gcf_cmp_x_1
        .globl  __gcf_cmp_x_2
        .globl  __gcf_cmp_x_3
        .globl  __gcf_cmp_x_4
        .globl  __gcf_cmp_x_5
        .globl  __gcf_cmp_x_6
        .globl  __gcf_cmp_x_7
        .globl  __gcf_cmp_x_8
        .globl  __gcf_cmp_x_half
        .globl  __gcf_finish
        .globl  __gcf_load_x
        .globl  __gcf_ret_nan
        .globl  __gcf_ret_pinf

        .area   _CODE
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

