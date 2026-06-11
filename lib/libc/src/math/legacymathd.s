        ;; legacymathd.s
        ;;
        ;; Real double / long double wrappers for the older non-transcendental
        ;; math entry points that historically aliased directly to the float
        ;; bodies. These wrappers convert through the existing single-precision
        ;; kernels until dedicated 64-bit double kernels exist.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module legacymathd
        .optsdcc -mz80 sdcccall(1)

        .globl  _fabs
        .globl  _fabsl
        .globl  _copysign
        .globl  _copysignl
        .globl  _trunc
        .globl  _truncl
        .globl  _floor
        .globl  _floorl
        .globl  _ceil
        .globl  _ceill
        .globl  _round
        .globl  _roundl
        .globl  _sqrt
        .globl  _sqrtl
        .globl  _atan2
        .globl  _atan2l
        .globl  _atan
        .globl  _atanl
        .globl  _asin
        .globl  _asinl
        .globl  _acos
        .globl  _acosl
        .globl  _sin
        .globl  _sinl
        .globl  _cos
        .globl  _cosl
        .globl  _tan
        .globl  _tanl
        .globl  _exp
        .globl  _expl
        .globl  _exp2
        .globl  _exp2l
        .globl  _expm1
        .globl  _expm1l
        .globl  _log
        .globl  _logl
        .globl  _log2
        .globl  _log2l
        .globl  _log10
        .globl  _log10l
        .globl  _log1p
        .globl  _log1pl
        .globl  _pow
        .globl  _powl
        .globl  _cbrt
        .globl  _cbrtl
        .globl  _ldexp
        .globl  _ldexpl
        .globl  _scalbn
        .globl  _scalbnl
        .globl  _frexp
        .globl  _frexpl
        .globl  _modf
        .globl  _modfl
        .globl  _ilogb
        .globl  _ilogbl
        .globl  _logb
        .globl  _logbl
        .globl  _fmax
        .globl  _fmaxl
        .globl  _fmin
        .globl  _fminl
        .globl  _fdim
        .globl  _fdiml
        .globl  _nan
        .globl  _nanl
        .globl  _significand

        .globl  ___db2fs
        .globl  ___fs2db
        .globl  _fabsf
        .globl  _copysignf
        .globl  _truncf
        .globl  _floorf
        .globl  _ceilf
        .globl  _roundf
        .globl  _sqrtf
        .globl  _atan2f
        .globl  _atanf
        .globl  _asinf
        .globl  _acosf
        .globl  _sinf
        .globl  _cosf
        .globl  _tanf
        .globl  _expf
        .globl  _exp2f
        .globl  _expm1f
        .globl  _logf
        .globl  _log2f
        .globl  _log10f
        .globl  _log1pf
        .globl  _powf
        .globl  _cbrtf
        .globl  _ldexpf
        .globl  _scalbnf
        .globl  _frexpf
        .globl  _modff
        .globl  _ilogbf
        .globl  _logbf
        .globl  _fmaxf
        .globl  _fminf
        .globl  _fdimf
        .globl  _nanf
        .globl  _significandf
        .globl  ___fssub

        .area   _CODE

LGD_XLO .equ -12
LGD_XHI .equ -10
LGD_YLO .equ -8
LGD_YHI .equ -6
LGD_ILO .equ -4
LGD_IHI .equ -2

__lgd_load_arg0_raw:
        ld      a,4(ix)
        ld      e,a
        ld      a,5(ix)
        ld      d,a
        ld      a,6(ix)
        ld      l,a
        ld      a,7(ix)
        ld      h,a
        exx
        ld      a,8(ix)
        ld      e,a
        ld      a,9(ix)
        ld      d,a
        ld      a,10(ix)
        ld      l,a
        ld      a,11(ix)
        ld      h,a
        exx
        ret

__lgd_load_arg1_raw:
        ld      a,12(ix)
        ld      e,a
        ld      a,13(ix)
        ld      d,a
        ld      a,14(ix)
        ld      l,a
        ld      a,15(ix)
        ld      h,a
        exx
        ld      a,16(ix)
        ld      e,a
        ld      a,17(ix)
        ld      d,a
        ld      a,18(ix)
        ld      l,a
        ld      a,19(ix)
        ld      h,a
        exx
        ret

__lgd_load_arg0_fs:
        call    __lgd_load_arg0_raw
        jp      ___db2fs

__lgd_load_arg1_fs:
        call    __lgd_load_arg1_raw
        jp      ___db2fs

__lgd_store_result_at_bc:
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
        inc     bc
        push    bc
        exx
        pop     bc
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
        exx
        ret

_fabs::
_fabsl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_raw
        exx
        res     7,h
        exx
        pop     ix
        ret

_copysign::
_copysignl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_raw
        exx
        res     7,h
        bit     7,19(ix)
        jr      z,lgd_copysign_done
        set     7,h
lgd_copysign_done:
        exx
        pop     ix
        ret

_trunc::
_truncl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _truncf
        call    ___fs2db
        pop     ix
        ret

_floor::
_floorl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _floorf
        call    ___fs2db
        pop     ix
        ret

_ceil::
_ceill::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _ceilf
        call    ___fs2db
        pop     ix
        ret

_round::
_roundl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _roundf
        call    ___fs2db
        pop     ix
        ret

_sqrt::
_sqrtl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _sqrtf
        call    ___fs2db
        pop     ix
        ret

_atan2::
_atan2l::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __lgd_load_arg0_fs
        ld      LGD_XLO(ix),e
        ld      LGD_XLO+1(ix),d
        ld      LGD_XHI(ix),l
        ld      LGD_XHI+1(ix),h
        call    __lgd_load_arg1_fs
        ld      LGD_YLO(ix),e
        ld      LGD_YLO+1(ix),d
        ld      LGD_YHI(ix),l
        ld      LGD_YHI+1(ix),h
        ld      l,LGD_YHI(ix)
        ld      h,LGD_YHI+1(ix)
        push    hl
        ld      e,LGD_YLO(ix)
        ld      d,LGD_YLO+1(ix)
        push    de
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    _atan2f
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_atan::
_atanl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _atanf
        call    ___fs2db
        pop     ix
        ret

_asin::
_asinl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _asinf
        call    ___fs2db
        pop     ix
        ret

_acos::
_acosl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _acosf
        call    ___fs2db
        pop     ix
        ret

_sin::
_sinl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _sinf
        call    ___fs2db
        pop     ix
        ret

_cos::
_cosl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _cosf
        call    ___fs2db
        pop     ix
        ret

_tan::
_tanl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _tanf
        call    ___fs2db
        pop     ix
        ret

_exp::
_expl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _expf
        call    ___fs2db
        pop     ix
        ret

_exp2::
_exp2l::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _exp2f
        call    ___fs2db
        pop     ix
        ret

_expm1::
_expm1l::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _expm1f
        call    ___fs2db
        pop     ix
        ret

_log::
_logl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _logf
        call    ___fs2db
        pop     ix
        ret

_log2::
_log2l::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _log2f
        call    ___fs2db
        pop     ix
        ret

_log10::
_log10l::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _log10f
        call    ___fs2db
        pop     ix
        ret

_log1p::
_log1pl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _log1pf
        call    ___fs2db
        pop     ix
        ret

_pow::
_powl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __lgd_load_arg0_fs
        ld      LGD_XLO(ix),e
        ld      LGD_XLO+1(ix),d
        ld      LGD_XHI(ix),l
        ld      LGD_XHI+1(ix),h
        call    __lgd_load_arg1_fs
        ld      LGD_YLO(ix),e
        ld      LGD_YLO+1(ix),d
        ld      LGD_YHI(ix),l
        ld      LGD_YHI+1(ix),h
        ld      l,LGD_YHI(ix)
        ld      h,LGD_YHI+1(ix)
        push    hl
        ld      e,LGD_YLO(ix)
        ld      d,LGD_YLO+1(ix)
        push    de
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    _powf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_cbrt::
_cbrtl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _cbrtf
        call    ___fs2db
        pop     ix
        ret

_ldexp::
_ldexpl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __lgd_load_arg0_fs
        ld      LGD_XLO(ix),e
        ld      LGD_XLO+1(ix),d
        ld      LGD_XHI(ix),l
        ld      LGD_XHI+1(ix),h
        ld      a,13(ix)
        ld      h,a
        ld      a,12(ix)
        ld      l,a
        push    hl
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    _ldexpf
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_scalbn::
_scalbnl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __lgd_load_arg0_fs
        ld      LGD_XLO(ix),e
        ld      LGD_XLO+1(ix),d
        ld      LGD_XHI(ix),l
        ld      LGD_XHI+1(ix),h
        ld      a,13(ix)
        ld      h,a
        ld      a,12(ix)
        ld      l,a
        push    hl
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    _scalbnf
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_frexp::
_frexpl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        ld      c,12(ix)
        ld      b,13(ix)
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        jr      z,lgd_frexp_exp_ok
        inc     a
lgd_frexp_exp_ok:
        or      a
        jr      z,lgd_frexp_zero
        sub     #126
        ld      (bc),a
        inc     bc
        rla
        sbc     a,a
        ld      (bc),a
        ld      a,h
        and     #0x80
        or      #0x3f
        ld      h,a
        ld      a,l
        and     #0x7f
        ld      l,a
        jr      lgd_frexp_ret
lgd_frexp_zero:
        xor     a
        ld      (bc),a
        inc     bc
        ld      (bc),a
lgd_frexp_ret:
        call    ___fs2db
        pop     ix
        ret

_modf::
_modfl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __lgd_load_arg0_fs
        ld      LGD_XLO(ix),e
        ld      LGD_XLO+1(ix),d
        ld      LGD_XHI(ix),l
        ld      LGD_XHI+1(ix),h
        call    _truncf
        ld      LGD_ILO(ix),e
        ld      LGD_ILO+1(ix),d
        ld      LGD_IHI(ix),l
        ld      LGD_IHI+1(ix),h
        call    ___fs2db
        ld      c,12(ix)
        ld      b,13(ix)
        call    __lgd_store_result_at_bc
        ld      l,LGD_IHI(ix)
        ld      h,LGD_IHI+1(ix)
        push    hl
        ld      e,LGD_ILO(ix)
        ld      d,LGD_ILO+1(ix)
        push    de
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    ___fssub
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_ilogb::
_ilogbl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        pop     ix
        jp      _ilogbf

_logb::
_logbl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _logbf
        call    ___fs2db
        pop     ix
        ret

_fmax::
_fmaxl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __lgd_load_arg0_fs
        ld      LGD_XLO(ix),e
        ld      LGD_XLO+1(ix),d
        ld      LGD_XHI(ix),l
        ld      LGD_XHI+1(ix),h
        call    __lgd_load_arg1_fs
        ld      LGD_YLO(ix),e
        ld      LGD_YLO+1(ix),d
        ld      LGD_YHI(ix),l
        ld      LGD_YHI+1(ix),h
        ld      l,LGD_YHI(ix)
        ld      h,LGD_YHI+1(ix)
        push    hl
        ld      e,LGD_YLO(ix)
        ld      d,LGD_YLO+1(ix)
        push    de
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    _fmaxf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_fmin::
_fminl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __lgd_load_arg0_fs
        ld      LGD_XLO(ix),e
        ld      LGD_XLO+1(ix),d
        ld      LGD_XHI(ix),l
        ld      LGD_XHI+1(ix),h
        call    __lgd_load_arg1_fs
        ld      LGD_YLO(ix),e
        ld      LGD_YLO+1(ix),d
        ld      LGD_YHI(ix),l
        ld      LGD_YHI+1(ix),h
        ld      l,LGD_YHI(ix)
        ld      h,LGD_YHI+1(ix)
        push    hl
        ld      e,LGD_YLO(ix)
        ld      d,LGD_YLO+1(ix)
        push    de
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    _fminf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_fdim::
_fdiml::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __lgd_load_arg0_fs
        ld      LGD_XLO(ix),e
        ld      LGD_XLO+1(ix),d
        ld      LGD_XHI(ix),l
        ld      LGD_XHI+1(ix),h
        call    __lgd_load_arg1_fs
        ld      LGD_YLO(ix),e
        ld      LGD_YLO+1(ix),d
        ld      LGD_YHI(ix),l
        ld      LGD_YHI+1(ix),h
        ld      l,LGD_YHI(ix)
        ld      h,LGD_YHI+1(ix)
        push    hl
        ld      e,LGD_YLO(ix)
        ld      d,LGD_YLO+1(ix)
        push    de
        ld      e,LGD_XLO(ix)
        ld      d,LGD_XLO+1(ix)
        ld      l,LGD_XHI(ix)
        ld      h,LGD_XHI+1(ix)
        call    _fdimf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_nan::
_nanl::
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x7ff8
        exx
        ret

_significand::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __lgd_load_arg0_fs
        call    _significandf
        call    ___fs2db
        pop     ix
        ret
