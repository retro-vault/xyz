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

        .area   _DATA
__md_x: .ds 4
__md_y: .ds 4
__md_z: .ds 4

        .area   _CODE

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
        call    __db_load_arg0_fs
        ld      (__md_x),de
        ld      (__md_x + 2),hl
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
        ld      de,(__md_x)
        ld      hl,(__md_x + 2)
        call    _scalblnf
        pop     bc
        pop     bc
        call    ___fs2db
        pop     ix
        ret

_fma::
_fmal::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        ld      (__md_x),de
        ld      (__md_x + 2),hl
        call    __db_load_arg1_fs
        ld      (__md_y),de
        ld      (__md_y + 2),hl
        call    __db_load_arg2_fs
        ld      (__md_z),de
        ld      (__md_z + 2),hl
        ld      hl,(__md_z + 2)
        push    hl
        ld      hl,(__md_z)
        push    hl
        ld      hl,(__md_y + 2)
        push    hl
        ld      hl,(__md_y)
        push    hl
        ld      de,(__md_x)
        ld      hl,(__md_x + 2)
        call    _fmaf
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        call    ___fs2db
        pop     ix
        ret

_hypot::
_hypotl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        ld      (__md_x),de
        ld      (__md_x + 2),hl
        call    __db_load_arg1_fs
        ld      (__md_y),de
        ld      (__md_y + 2),hl
        ld      hl,(__md_y + 2)
        push    hl
        ld      hl,(__md_y)
        push    hl
        ld      de,(__md_x)
        ld      hl,(__md_x + 2)
        call    _hypotf
        pop     bc
        pop     bc
        call    ___fs2db
        pop     ix
        ret

_fmod::
_fmodl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        ld      (__md_x),de
        ld      (__md_x + 2),hl
        call    __db_load_arg1_fs
        ld      (__md_y),de
        ld      (__md_y + 2),hl
        ld      hl,(__md_y + 2)
        push    hl
        ld      hl,(__md_y)
        push    hl
        ld      de,(__md_x)
        ld      hl,(__md_x + 2)
        call    _fmodf
        pop     bc
        pop     bc
        call    ___fs2db
        pop     ix
        ret

_remainder::
_remainderl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        ld      (__md_x),de
        ld      (__md_x + 2),hl
        call    __db_load_arg1_fs
        ld      (__md_y),de
        ld      (__md_y + 2),hl
        ld      hl,(__md_y + 2)
        push    hl
        ld      hl,(__md_y)
        push    hl
        ld      de,(__md_x)
        ld      hl,(__md_x + 2)
        call    _remainderf
        pop     bc
        pop     bc
        call    ___fs2db
        pop     ix
        ret

_remquo::
_remquol::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        ld      (__md_x),de
        ld      (__md_x + 2),hl
        call    __db_load_arg1_fs
        ld      (__md_y),de
        ld      (__md_y + 2),hl
        ld      a,21(ix)
        ld      h,a
        ld      a,20(ix)
        ld      l,a
        push    hl
        ld      hl,(__md_y + 2)
        push    hl
        ld      hl,(__md_y)
        push    hl
        ld      de,(__md_x)
        ld      hl,(__md_x + 2)
        call    _remquof
        pop     bc
        pop     bc
        pop     bc
        call    ___fs2db
        pop     ix
        ret

_nextafter::
_nextafterl::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __db_load_arg0_fs
        ld      (__md_x),de
        ld      (__md_x + 2),hl
        call    __db_load_arg1_fs
        ld      (__md_y),de
        ld      (__md_y + 2),hl
        ld      hl,(__md_y + 2)
        push    hl
        ld      hl,(__md_y)
        push    hl
        ld      de,(__md_x)
        ld      hl,(__md_x + 2)
        call    _nextafterf
        pop     bc
        pop     bc
        call    ___fs2db
        pop     ix
        ret
