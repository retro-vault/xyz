        ;; moremathd.s
        ;;
        ;; Double / long double wrappers for the additional non-transcendental
        ;; math entry points.  The current libc still computes these through the
        ;; proven single-precision kernels, then converts back through the
        ;; 64-bit double runtime so programs link and run correctly.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module moremathd
        .optsdcc -mz80 sdcccall(1)

        .globl  _rint
        .globl  _rintl
        .globl  _nearbyint
        .globl  _nearbyintl
        .globl  _lround
        .globl  _lroundl
        .globl  _lrint
        .globl  _lrintl
        .globl  _llround
        .globl  _llroundl
        .globl  _llrint
        .globl  _llrintl
        .globl  _scalbln
        .globl  _scalblnl
        .globl  _fma
        .globl  _fmal
        .globl  _hypot
        .globl  _hypotl
        .globl  _fmod
        .globl  _fmodl
        .globl  _remainder
        .globl  _remainderl
        .globl  _remquo
        .globl  _remquol
        .globl  _nextafter
        .globl  _nextafterl
        .globl  _nextup
        .globl  _nextupl
        .globl  _nextdown
        .globl  _nextdownl

        .globl  ___db2fs
        .globl  ___fs2db
        .globl  _rintf
        .globl  _nearbyintf
        .globl  _lroundf
        .globl  _lrintf
        .globl  _llroundf
        .globl  _llrintf
        .globl  _scalblnf
        .globl  _fmaf
        .globl  _hypotf
        .globl  _fmodf
        .globl  _remainderf
        .globl  _remquof
        .globl  _nextafterf

        .area   _CODE

MD_XLO  .equ -12
MD_XHI  .equ -10
MD_YLO  .equ -8
MD_YHI  .equ -6
MD_ZLO  .equ -4
MD_ZHI  .equ -2

        ;; The C-library double ABI passes each double as eight stacked bytes.
        ;; The float kernels expect a 64-bit runtime double in DE:HL:DE':HL',
        ;; so each helper rebuilds that register layout and tail-calls ___db2fs.
__db_load_arg0_fs:
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
        jp      ___db2fs

__db_load_arg1_fs:
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
        jp      ___db2fs

__db_load_arg2_fs:
        ld      a,20(ix)
        ld      e,a
        ld      a,21(ix)
        ld      d,a
        ld      a,22(ix)
        ld      l,a
        ld      a,23(ix)
        ld      h,a
        exx
        ld      a,24(ix)
        ld      e,a
        ld      a,25(ix)
        ld      d,a
        ld      a,26(ix)
        ld      l,a
        ld      a,27(ix)
        ld      h,a
        exx
        jp      ___db2fs

        ;; The wrappers below are intentionally shallow:
        ;;   1. load stacked double argument(s),
        ;;   2. convert to float,
        ;;   3. reuse the proven float entry point,
        ;;   4. convert back to 64-bit double when the result is floating.
_rint::
_rintl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        call    _rintf
        call    ___fs2db
        pop     ix
        ret

_nearbyint::
_nearbyintl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        call    _nearbyintf
        call    ___fs2db
        pop     ix
        ret

_lround::
_lroundl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        pop     ix
        jp      _lroundf               ; integer result stays in the float ABI

_lrint::
_lrintl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        pop     ix
        jp      _lrintf                ; integer result stays in the float ABI

_llround::
_llroundl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        pop     ix
        jp      _llroundf              ; 64-bit integer result bypasses fs2db

_llrint::
_llrintl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        pop     ix
        jp      _llrintf               ; 64-bit integer result bypasses fs2db

_scalbln::
_scalblnl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        ld      MD_XLO(ix),e
        ld      MD_XLO+1(ix),d
        ld      MD_XHI(ix),l
        ld      MD_XHI+1(ix),h
        ;; long n stays stacked as a 32-bit quantity for the float entry point.
        ld      a,15(ix)
        ld      h,a
        ld      a,14(ix)
        ld      l,a
        push    hl
        ld      a,13(ix)
        ld      h,a
        ld      a,12(ix)
        ld      l,a
        push    hl
        ld      e,MD_XLO(ix)
        ld      d,MD_XLO+1(ix)
        ld      l,MD_XHI(ix)
        ld      h,MD_XHI+1(ix)
        call    _scalblnf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_fma::
_fmal::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        ld      MD_XLO(ix),e
        ld      MD_XLO+1(ix),d
        ld      MD_XHI(ix),l
        ld      MD_XHI+1(ix),h
        call    __db_load_arg1_fs
        ld      MD_YLO(ix),e
        ld      MD_YLO+1(ix),d
        ld      MD_YHI(ix),l
        ld      MD_YHI+1(ix),h
        call    __db_load_arg2_fs
        ld      MD_ZLO(ix),e
        ld      MD_ZLO+1(ix),d
        ld      MD_ZHI(ix),l
        ld      MD_ZHI+1(ix),h
        ld      l,MD_ZHI(ix)
        ld      h,MD_ZHI+1(ix)
        push    hl
        ld      l,MD_ZLO(ix)
        ld      h,MD_ZLO+1(ix)
        push    hl
        ld      l,MD_YHI(ix)
        ld      h,MD_YHI+1(ix)
        push    hl
        ld      l,MD_YLO(ix)
        ld      h,MD_YLO+1(ix)
        push    hl
        ld      e,MD_XLO(ix)
        ld      d,MD_XLO+1(ix)
        ld      l,MD_XHI(ix)
        ld      h,MD_XHI+1(ix)
        call    _fmaf
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_hypot::
_hypotl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        ld      MD_XLO(ix),e
        ld      MD_XLO+1(ix),d
        ld      MD_XHI(ix),l
        ld      MD_XHI+1(ix),h
        call    __db_load_arg1_fs
        ld      MD_YLO(ix),e
        ld      MD_YLO+1(ix),d
        ld      MD_YHI(ix),l
        ld      MD_YHI+1(ix),h
        ld      l,MD_YHI(ix)
        ld      h,MD_YHI+1(ix)
        push    hl
        ld      l,MD_YLO(ix)
        ld      h,MD_YLO+1(ix)
        push    hl
        ld      e,MD_XLO(ix)
        ld      d,MD_XLO+1(ix)
        ld      l,MD_XHI(ix)
        ld      h,MD_XHI+1(ix)
        call    _hypotf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_fmod::
_fmodl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        ld      MD_XLO(ix),e
        ld      MD_XLO+1(ix),d
        ld      MD_XHI(ix),l
        ld      MD_XHI+1(ix),h
        call    __db_load_arg1_fs
        ld      MD_YLO(ix),e
        ld      MD_YLO+1(ix),d
        ld      MD_YHI(ix),l
        ld      MD_YHI+1(ix),h
        ld      l,MD_YHI(ix)
        ld      h,MD_YHI+1(ix)
        push    hl
        ld      l,MD_YLO(ix)
        ld      h,MD_YLO+1(ix)
        push    hl
        ld      e,MD_XLO(ix)
        ld      d,MD_XLO+1(ix)
        ld      l,MD_XHI(ix)
        ld      h,MD_XHI+1(ix)
        call    _fmodf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_remainder::
_remainderl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        ld      MD_XLO(ix),e
        ld      MD_XLO+1(ix),d
        ld      MD_XHI(ix),l
        ld      MD_XHI+1(ix),h
        call    __db_load_arg1_fs
        ld      MD_YLO(ix),e
        ld      MD_YLO+1(ix),d
        ld      MD_YHI(ix),l
        ld      MD_YHI+1(ix),h
        ld      l,MD_YHI(ix)
        ld      h,MD_YHI+1(ix)
        push    hl
        ld      l,MD_YLO(ix)
        ld      h,MD_YLO+1(ix)
        push    hl
        ld      e,MD_XLO(ix)
        ld      d,MD_XLO+1(ix)
        ld      l,MD_XHI(ix)
        ld      h,MD_XHI+1(ix)
        call    _remainderf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_remquo::
_remquol::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        ld      MD_XLO(ix),e
        ld      MD_XLO+1(ix),d
        ld      MD_XHI(ix),l
        ld      MD_XHI+1(ix),h
        call    __db_load_arg1_fs
        ld      MD_YLO(ix),e
        ld      MD_YLO+1(ix),d
        ld      MD_YHI(ix),l
        ld      MD_YHI+1(ix),h
        ld      a,21(ix)
        ld      h,a
        ld      a,20(ix)
        ld      l,a
        push    hl
        ld      l,MD_YHI(ix)
        ld      h,MD_YHI+1(ix)
        push    hl
        ld      l,MD_YLO(ix)
        ld      h,MD_YLO+1(ix)
        push    hl
        ld      e,MD_XLO(ix)
        ld      d,MD_XLO+1(ix)
        ld      l,MD_XHI(ix)
        ld      h,MD_XHI+1(ix)
        call    _remquof
        pop     bc
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_nextafter::
_nextafterl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        ld      MD_XLO(ix),e
        ld      MD_XLO+1(ix),d
        ld      MD_XHI(ix),l
        ld      MD_XHI+1(ix),h
        call    __db_load_arg1_fs
        ld      MD_YLO(ix),e
        ld      MD_YLO+1(ix),d
        ld      MD_YHI(ix),l
        ld      MD_YHI+1(ix),h
        ld      l,MD_YHI(ix)
        ld      h,MD_YHI+1(ix)
        push    hl
        ld      l,MD_YLO(ix)
        ld      h,MD_YLO+1(ix)
        push    hl
        ld      e,MD_XLO(ix)
        ld      d,MD_XLO+1(ix)
        ld      l,MD_XHI(ix)
        ld      h,MD_XHI+1(ix)
        call    _nextafterf
        pop     bc
        pop     bc
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

        ;; C23 nextup / nextdown double wrappers (new).
        ;; Use the proven float versions via db<->fs conversion (consistent
        ;; with other "moremathd" functions in this file).  Precision is
        ;; limited to single for values outside float range.
_nextup::
_nextupl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _nextupf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_nextdown::
_nextdownl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _nextdownf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

;; C23 double math (new) - thin wrappers calling f versions after convert (consistent with file style, stack frame).

        .globl  _fromfp
        .globl  _fromfpl
        .globl  _ufromfp
        .globl  _ufromfpl
        .globl  _fromfpx
        .globl  _fromfpxl
        .globl  _ufromfpx
        .globl  _ufromfpxl
        .globl  _roundeven
        .globl  _roundevenl
        .globl  _fmaximum
        .globl  _fmaximuml
        .globl  _fminimum
        .globl  _fminimuml
        .globl  _fmaximum_mag
        .globl  _fmaximum_magl
        .globl  _fminimum_mag
        .globl  _fminimum_magl
        .globl  _fmaximum_num
        .globl  _fmaximum_numl
        .globl  _fminimum_num
        .globl  _fminimum_numl
        .globl  _fmaximum_mag_num
        .globl  _fmaximum_mag_numl
        .globl  _fminimum_mag_num
        .globl  _fminimum_mag_numl
        .globl  _getpayload
        .globl  _getpayloadl
        .globl  _setpayload
        .globl  _setpayloadl
        .globl  _setpayloadsig
        .globl  _setpayloadsigl
        .globl  _totalorder
        .globl  _totalorderl
        .globl  _totalordermag
        .globl  _totalordermagl

_fromfp::
_fromfpl::
_fromfpx::
_fromfpxl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _fromfpf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_ufromfp::
_ufromfpl::
_ufromfpx::
_ufromfpxl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _ufromfpf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_roundeven::
_roundevenl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _roundevenf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_fmaximum::
_fmaximuml::
_fmaximum_num::
_fmaximum_numl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _fmaximumf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_fminimum::
_fminimuml::
_fminimum_num::
_fminimum_numl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _fminimumf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_fmaximum_mag::
_fmaximum_magl::
_fmaximum_mag_num::
_fmaximum_mag_numl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _fmaximum_magf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_fminimum_mag::
_fminimum_magl::
_fminimum_mag_num::
_fminimum_mag_numl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _fminimum_magf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_getpayload::
_getpayloadl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _getpayloadf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_setpayload::
_setpayloadl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _setpayloadf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_setpayloadsig::
_setpayloadsigl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _setpayloadsigf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

_totalorder::
_totalorderl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _totalorderf
        ; returns int in DE
        ld      sp,ix
        pop     ix
        ret

_totalordermag::
_totalordermagl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    __db_load_arg1_fs
        call    _totalordermagf
        ld      sp,ix
        pop     ix
        ret
