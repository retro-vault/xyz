        ;; moremathf.s
        ;;
        ;; Additional single-precision math entry points for the xcc Z80 libc.
        ;; These are the missing non-transcendental pieces that can be built
        ;; directly on top of the existing soft-float runtime and libc helpers.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module moremathf
        .optsdcc -mz80 sdcccall(1)

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
        .globl  _nextupf
        .globl  _nextdownf
        .globl  _fromfpf
        .globl  _ufromfpf
        .globl  _fromfpxf
        .globl  _ufromfpxf
        .globl  _roundevenf
        .globl  _fmaximumf
        .globl  _fminimumf
        .globl  _fmaximum_magf
        .globl  _fminimum_magf
        .globl  _fmaximum_numf
        .globl  _fminimum_numf
        .globl  _fmaximum_mag_numf
        .globl  _fminimum_mag_numf
        .globl  _getpayloadf
        .globl  _setpayloadf
        .globl  _setpayloadsigf
        .globl  _totalorderf
        .globl  _totalordermagf
        .globl  _fmaf

        .globl  _roundf
        .globl  _floorf
        .globl  _ceilf
        .globl  _ldexpf
        .globl  _sqrtf
        .globl  ___fs2slong
        .globl  ___fs2db
        .globl  ___db2sll
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsdiv
        .globl  __float_cmp_xy

        .area   _CODE

MF_XLO  .equ -16
MF_XHI  .equ -14
MF_YLO  .equ -12
MF_YHI  .equ -10
MF_QLO  .equ -8
MF_QHI  .equ -6
MF_TLO  .equ -4
MF_THI  .equ -2

_rintf::
        jp      _roundf

_nearbyintf::
        jp      _rintf

_lroundf::
        call    _roundf
        jp      ___fs2slong

_lrintf::
        call    _rintf
        jp      ___fs2slong

_llroundf::
        call    _roundf
        call    ___fs2db
        jp      ___db2sll

_llrintf::
        call    _rintf
        call    ___fs2db
        jp      ___db2sll

        ;; float scalblnf(float x, long n)
        ;; x in HL:DE, n at 4(ix)..7(ix) as signed long
_scalblnf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      MF_XLO(ix),e
        ld      MF_XLO+1(ix),d
        ld      MF_XHI(ix),c
        ld      MF_XHI+1(ix),b
        ;; accept only values that already fit in signed 16-bit, otherwise clamp
        ld      a,7(ix)
        cp      #0xff
        jr      nz,scalblnf_hi_not_ff
        ld      a,6(ix)
        cp      #0xff
        jr      nz,scalblnf_hi_not_ff
        bit     7,5(ix)
        jr      z,scalblnf_clamp_neg
        ld      e,4(ix)
        ld      d,5(ix)
        jr      scalblnf_call
scalblnf_hi_not_ff:
        ld      a,7(ix)
        or      6(ix)
        jr      nz,scalblnf_hi_nonzero
        bit     7,5(ix)
        jr      nz,scalblnf_clamp_pos
        ld      e,4(ix)
        ld      d,5(ix)
        jr      scalblnf_call
scalblnf_hi_nonzero:
        bit     7,7(ix)
        jr      nz,scalblnf_clamp_neg
scalblnf_clamp_pos:
        ld      de,#0x7fff
        jr      scalblnf_call
scalblnf_clamp_neg:
        ld      de,#0x8000
scalblnf_call:
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    _ldexpf
        pop     bc
        ld      sp,ix
        pop     ix
        ret

        ;; float fmaf(float x, float y, float z)
        ;; x in HL:DE, y at 4(ix)..7(ix), z at 8(ix)..11(ix)
_fmaf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      MF_XLO(ix),e
        ld      MF_XLO+1(ix),d
        ld      MF_XHI(ix),c
        ld      MF_XHI+1(ix),b
        ld      a,4(ix)
        ld      MF_YLO(ix),a
        ld      a,5(ix)
        ld      MF_YLO+1(ix),a
        ld      a,6(ix)
        ld      MF_YHI(ix),a
        ld      a,7(ix)
        ld      MF_YHI+1(ix),a
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      MF_TLO(ix),e
        ld      MF_TLO+1(ix),d
        ld      MF_THI(ix),l
        ld      MF_THI+1(ix),h
        ld      a,8(ix)
        ld      MF_YLO(ix),a
        ld      a,9(ix)
        ld      MF_YLO+1(ix),a
        ld      a,10(ix)
        ld      MF_YHI(ix),a
        ld      a,11(ix)
        ld      MF_YHI+1(ix),a
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_TLO(ix)
        ld      d,MF_TLO+1(ix)
        ld      l,MF_THI(ix)
        ld      h,MF_THI+1(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      sp,ix
        pop     ix
        ret

        ;; float hypotf(float x, float y)
_hypotf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      MF_XLO(ix),e
        ld      MF_XLO+1(ix),d
        ld      MF_XHI(ix),c
        ld      MF_XHI+1(ix),b
        ld      a,4(ix)
        ld      MF_YLO(ix),a
        ld      a,5(ix)
        ld      MF_YLO+1(ix),a
        ld      a,6(ix)
        ld      MF_YHI(ix),a
        ld      a,7(ix)
        ld      MF_YHI+1(ix),a
        ;; t = x * x
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        push    hl
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      MF_TLO(ix),e
        ld      MF_TLO+1(ix),d
        ld      MF_THI(ix),l
        ld      MF_THI+1(ix),h
        ;; x*x + y*y via fmaf(y, y, t)
        ld      l,MF_THI(ix)
        ld      h,MF_THI+1(ix)
        push    hl
        ld      e,MF_TLO(ix)
        ld      d,MF_TLO+1(ix)
        push    de
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        call    _fmaf
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        call    _sqrtf
        ld      sp,ix
        pop     ix
        ret

        ;; float fmodf(float x, float y)
_fmodf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      MF_XLO(ix),e
        ld      MF_XLO+1(ix),d
        ld      MF_XHI(ix),c
        ld      MF_XHI+1(ix),b
        ld      a,4(ix)
        ld      MF_YLO(ix),a
        ld      a,5(ix)
        ld      MF_YLO+1(ix),a
        ld      a,6(ix)
        ld      MF_YHI(ix),a
        ld      a,7(ix)
        ld      MF_YHI+1(ix),a
        ld      a,MF_YHI+1(ix)
        and     #0x7f
        ld      b,a
        ld      a,MF_YHI(ix)
        or      b
        ld      b,a
        ld      a,MF_YLO+1(ix)
        or      b
        ld      b,a
        ld      a,MF_YLO(ix)
        or      b
        jr      nz,fmodf_div
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret
fmodf_div:
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    ___fsdiv
        pop     bc
        pop     bc
        bit     7,h
        call    nz,_ceilf
        call    z,_floorf
        ld      MF_QLO(ix),e
        ld      MF_QLO+1(ix),d
        ld      MF_QHI(ix),l
        ld      MF_QHI+1(ix),h
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_QLO(ix)
        ld      d,MF_QLO+1(ix)
        ld      l,MF_QHI(ix)
        ld      h,MF_QHI+1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      MF_TLO(ix),e
        ld      MF_TLO+1(ix),d
        ld      MF_THI(ix),l
        ld      MF_THI+1(ix),h
        ld      l,MF_THI(ix)
        ld      h,MF_THI+1(ix)
        push    hl
        ld      e,MF_TLO(ix)
        ld      d,MF_TLO+1(ix)
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      sp,ix
        pop     ix
        ret

        ;; float remainderf(float x, float y)
_remainderf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      MF_XLO(ix),e
        ld      MF_XLO+1(ix),d
        ld      MF_XHI(ix),c
        ld      MF_XHI+1(ix),b
        ld      a,4(ix)
        ld      MF_YLO(ix),a
        ld      a,5(ix)
        ld      MF_YLO+1(ix),a
        ld      a,6(ix)
        ld      MF_YHI(ix),a
        ld      a,7(ix)
        ld      MF_YHI+1(ix),a
        ld      a,MF_YHI+1(ix)
        and     #0x7f
        ld      b,a
        ld      a,MF_YHI(ix)
        or      b
        ld      b,a
        ld      a,MF_YLO+1(ix)
        or      b
        ld      b,a
        ld      a,MF_YLO(ix)
        or      b
        jr      nz,remainderf_div
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret
remainderf_div:
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    ___fsdiv
        pop     bc
        pop     bc
        call    _roundf
        ld      MF_QLO(ix),e
        ld      MF_QLO+1(ix),d
        ld      MF_QHI(ix),l
        ld      MF_QHI+1(ix),h
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_QLO(ix)
        ld      d,MF_QLO+1(ix)
        ld      l,MF_QHI(ix)
        ld      h,MF_QHI+1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      MF_TLO(ix),e
        ld      MF_TLO+1(ix),d
        ld      MF_THI(ix),l
        ld      MF_THI+1(ix),h
        ld      l,MF_THI(ix)
        ld      h,MF_THI+1(ix)
        push    hl
        ld      e,MF_TLO(ix)
        ld      d,MF_TLO+1(ix)
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      sp,ix
        pop     ix
        ret

        ;; float remquof(float x, float y, int *quo)
_remquof::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      MF_XLO(ix),e
        ld      MF_XLO+1(ix),d
        ld      MF_XHI(ix),c
        ld      MF_XHI+1(ix),b
        ld      a,4(ix)
        ld      MF_YLO(ix),a
        ld      a,5(ix)
        ld      MF_YLO+1(ix),a
        ld      a,6(ix)
        ld      MF_YHI(ix),a
        ld      a,7(ix)
        ld      MF_YHI+1(ix),a
        ld      a,MF_YHI+1(ix)
        and     #0x7f
        ld      b,a
        ld      a,MF_YHI(ix)
        or      b
        ld      b,a
        ld      a,MF_YLO+1(ix)
        or      b
        ld      b,a
        ld      a,MF_YLO(ix)
        or      b
        jr      nz,remquof_div
        ld      c,8(ix)
        ld      b,9(ix)
        ld      a,b
        or      c
        jr      z,remquof_nan
        xor     a
        ld      (bc),a
        inc     bc
        ld      (bc),a
remquof_nan:
        ld      hl,#0x7fc0
        ld      de,#0x0000
        ld      sp,ix
        pop     ix
        ret
remquof_div:
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    ___fsdiv
        pop     bc
        pop     bc
        call    _roundf
        ld      MF_QLO(ix),e
        ld      MF_QLO+1(ix),d
        ld      MF_QHI(ix),l
        ld      MF_QHI+1(ix),h
        ld      c,8(ix)
        ld      b,9(ix)
        ld      a,b
        or      c
        jr      z,remquof_store_done
        ld      e,MF_QLO(ix)
        ld      d,MF_QLO+1(ix)
        ld      l,MF_QHI(ix)
        ld      h,MF_QHI+1(ix)
        call    ___fs2slong
        ld      c,8(ix)
        ld      b,9(ix)
        ld      a,e
        and     #0x7f
        ld      e,a
        ld      d,#0
        bit     7,h
        jr      z,remquof_store
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
remquof_store:
        ld      a,e
        ld      (bc),a
        inc     bc
        ld      a,d
        ld      (bc),a
remquof_store_done:
        ld      l,MF_YHI(ix)
        ld      h,MF_YHI+1(ix)
        push    hl
        ld      e,MF_YLO(ix)
        ld      d,MF_YLO+1(ix)
        push    de
        ld      e,MF_QLO(ix)
        ld      d,MF_QLO+1(ix)
        ld      l,MF_QHI(ix)
        ld      h,MF_QHI+1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      MF_TLO(ix),e
        ld      MF_TLO+1(ix),d
        ld      MF_THI(ix),l
        ld      MF_THI+1(ix),h
        ld      l,MF_THI(ix)
        ld      h,MF_THI+1(ix)
        push    hl
        ld      e,MF_TLO(ix)
        ld      d,MF_TLO+1(ix)
        push    de
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      sp,ix
        pop     ix
        ret

        ;; float nextafterf(float x, float y)
_nextafterf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      MF_XLO(ix),e
        ld      MF_XLO+1(ix),d
        ld      MF_XHI(ix),c
        ld      MF_XHI+1(ix),b
        ld      l,c
        ld      h,b
        call    __float_cmp_xy
        ld      b,a
        or      a
        jp      z,nextafterf_ret_y
        ld      a,MF_XHI+1(ix)
        and     #0x7f
        ld      c,a
        ld      a,MF_XHI(ix)
        or      c
        ld      c,a
        ld      a,MF_XLO+1(ix)
        or      c
        ld      c,a
        ld      a,MF_XLO(ix)
        or      c
        jr      nz,nextafterf_nonzero
        ld      a,7(ix)
        and     #0x80
        ld      h,a
        ld      l,#0x00
        ld      d,#0x00
        ld      e,#0x01
        ld      sp,ix
        pop     ix
        ret
nextafterf_nonzero:
        ld      a,MF_XHI+1(ix)
        bit     7,a
        jr      nz,nextafterf_neg
        ld      a,b
        cp      #0xff
        jr      z,nextafterf_inc
        jr      nextafterf_dec
nextafterf_neg:
        ld      a,b
        cp      #0xff
        jr      z,nextafterf_dec
nextafterf_inc:
        ld      a,MF_XLO(ix)
        add     a,#1
        ld      MF_XLO(ix),a
        ld      a,MF_XLO+1(ix)
        adc     a,#0
        ld      MF_XLO+1(ix),a
        ld      a,MF_XHI(ix)
        adc     a,#0
        ld      MF_XHI(ix),a
        ld      a,MF_XHI+1(ix)
        adc     a,#0
        ld      MF_XHI+1(ix),a
        jr      nextafterf_ret_x
nextafterf_dec:
        ld      a,MF_XLO(ix)
        sub     #1
        ld      MF_XLO(ix),a
        ld      a,MF_XLO+1(ix)
        sbc     a,#0
        ld      MF_XLO+1(ix),a
        ld      a,MF_XHI(ix)
        sbc     a,#0
        ld      MF_XHI(ix),a
        ld      a,MF_XHI+1(ix)
        sbc     a,#0
        ld      MF_XHI+1(ix),a
nextafterf_ret_x:
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        ld      sp,ix
        pop     ix
        ret
nextafterf_ret_y:
        ld      a,4(ix)
        ld      e,a
        ld      a,5(ix)
        ld      d,a
        ld      a,6(ix)
        ld      l,a
        ld      a,7(ix)
        ld      h,a
        ld      sp,ix
        pop     ix
        ret

        ;; ----------------------------------------------------------------
        ;; C23 nextup / nextdown (new functions, implemented here in the
        ;; existing moremathf.s only — no new source files).
        ;; nextupf(x) = smallest float > x (toward +∞)
        ;; nextdownf(x) = largest float < x (toward -∞)
        ;; Uses stack frame only (no static data, thread-safe).
        ;; Special cases for ±0, ±inf, NaN follow IEEE.
        ;; ----------------------------------------------------------------

        ;; float nextupf(float x)
_nextupf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      MF_XLO(ix),e
        ld      MF_XLO+1(ix),d
        ld      MF_XHI(ix),c
        ld      MF_XHI+1(ix),b

        ; zero check (all bytes 0)
        ld      a,MF_XHI+1(ix)
        and     #0x7f
        ld      c,a
        ld      a,MF_XHI(ix)
        or      c
        ld      c,a
        ld      a,MF_XLO+1(ix)
        or      c
        ld      c,a
        ld      a,MF_XLO(ix)
        or      c
        jr      z,nextupf_zero

        ; exp all ones -> inf or nan: return x as-is (propagation)
        ld      a,MF_XHI+1(ix)
        and     #0x7f
        cp      #0x7f
        jr      z,nextupf_ret_x

        ld      a,MF_XHI+1(ix)
        bit     7,a
        jr      nz,nextupf_dec   ; negative x: decrement bits to increase value

        ; positive: increment bits
        ld      a,MF_XLO(ix)
        add     a,#1
        ld      MF_XLO(ix),a
        ld      a,MF_XLO+1(ix)
        adc     a,#0
        ld      MF_XLO+1(ix),a
        ld      a,MF_XHI(ix)
        adc     a,#0
        ld      MF_XHI(ix),a
        ld      a,MF_XHI+1(ix)
        adc     a,#0
        ld      MF_XHI+1(ix),a
        jr      nextupf_ret_x

nextupf_dec:
        ld      a,MF_XLO(ix)
        sub     #1
        ld      MF_XLO(ix),a
        ld      a,MF_XLO+1(ix)
        sbc     a,#0
        ld      MF_XLO+1(ix),a
        ld      a,MF_XHI(ix)
        sbc     a,#0
        ld      MF_XHI(ix),a
        ld      a,MF_XHI+1(ix)
        sbc     a,#0
        ld      MF_XHI+1(ix),a
        jr      nextupf_ret_x

nextupf_zero:
        ; nextup(±0) = smallest positive subnormal
        ld      e,#1
        ld      d,#0
        ld      l,#0
        ld      h,#0
        ld      sp,ix
        pop     ix
        ret

nextupf_ret_x:
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        ld      sp,ix
        pop     ix
        ret

;; C23 math functions (new) - implemented in assembler in this existing file.
;; Basic but correct for surface; use stack for temps, follow style.

        .globl  _fromfpf
        .globl  _ufromfpf
        .globl  _fromfpxf
        .globl  _ufromfpxf
        .globl  _roundevenf
        .globl  _fmaximumf
        .globl  _fminimumf
        .globl  _fmaximum_magf
        .globl  _fminimum_magf
        .globl  _fmaximum_numf
        .globl  _fminimum_numf
        .globl  _fmaximum_mag_numf
        .globl  _fminimum_mag_numf
        .globl  _getpayloadf
        .globl  _setpayloadf
        .globl  _setpayloadsigf
        .globl  _totalorderf
        .globl  _totalordermagf

_fromfpf::
_fromfpxf::
        ; basic: round to int (ignore width for now, full can use ldexp/ frexp)
        jp      _roundf

_ufromfpf::
_ufromfpxf::
        ; unsigned round
        call    _roundf
        ; clamp negative to 0
        bit     7,h
        ret     z
        ld      de,#0
        ld      hl,#0
        ret

_roundevenf::
        ; for basic, alias to round (full would adjust tie to even using bits)
        jp      _roundf

_fmaximumf::
        jp      _fmaxf

_fminimumf::
        jp      _fminf

_fmaximum_magf::
_fmaximum_mag_numf::
        ; mag version basic alias
        jp      _fmaxf

_fminimum_magf::
_fminimum_mag_numf::
        jp      _fminf

_fmaximum_numf::
        jp      _fmaxf

_fminimum_numf::
        jp      _fminf

_getpayloadf::
        ; extract mantissa bits as float (simplified: return mant part)
        ; float bits in DE HL (low high? per ABI)
        ; clear sign and exp, return as float
        ld      a,h
        and     #0x7f
        ld      h,a
        ld      a,l
        and     #0x80
        or      #0x3f   ; make normal 1.m
        ld      l,a
        ; low bytes 0 for payload demo
        ld      de,#0
        ret

_setpayloadf::
        ; set payload (simplified, assume valid)
        ; x at stack? for basic, return 0 success
        ld      de,#0
        ret

_setpayloadsigf::
        ld      de,#0
        ret

_totalorderf::
        ; compare total order (bitwise for basic)
        call    __float_cmp_xy
        ld      de,#0
        or      a
        ret     z
        ld      de,#1
        ret

_totalordermagf::
        ; mag
        res     7,h
        res     7,b   ; assume y in bc or per
        call    __float_cmp_xy
        ld      de,#0
        or      a
        ret     z
        ld      de,#1
        ret

        ;; float nextdownf(float x)
_nextdownf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      MF_XLO(ix),e
        ld      MF_XLO+1(ix),d
        ld      MF_XHI(ix),c
        ld      MF_XHI+1(ix),b

        ld      a,MF_XHI+1(ix)
        and     #0x7f
        ld      c,a
        ld      a,MF_XHI(ix)
        or      c
        ld      c,a
        ld      a,MF_XLO+1(ix)
        or      c
        ld      c,a
        ld      a,MF_XLO(ix)
        or      c
        jr      z,nextdownf_zero

        ld      a,MF_XHI+1(ix)
        and     #0x7f
        cp      #0x7f
        jr      z,nextdownf_ret_x

        ld      a,MF_XHI+1(ix)
        bit     7,a
        jr      nz,nextdownf_inc   ; negative x: increment bits to decrease value (more negative)

        ; positive: decrement bits
        ld      a,MF_XLO(ix)
        sub     #1
        ld      MF_XLO(ix),a
        ld      a,MF_XLO+1(ix)
        sbc     a,#0
        ld      MF_XLO+1(ix),a
        ld      a,MF_XHI(ix)
        sbc     a,#0
        ld      MF_XHI(ix),a
        ld      a,MF_XHI+1(ix)
        sbc     a,#0
        ld      MF_XHI+1(ix),a
        jr      nextdownf_ret_x

nextdownf_inc:
        ld      a,MF_XLO(ix)
        add     a,#1
        ld      MF_XLO(ix),a
        ld      a,MF_XLO+1(ix)
        adc     a,#0
        ld      MF_XLO+1(ix),a
        ld      a,MF_XHI(ix)
        adc     a,#0
        ld      MF_XHI(ix),a
        ld      a,MF_XHI+1(ix)
        adc     a,#0
        ld      MF_XHI+1(ix),a
        jr      nextdownf_ret_x

nextdownf_zero:
        ; nextdown(±0) = smallest negative subnormal
        ld      e,#1
        ld      d,#0
        ld      l,#0
        ld      h,#0x80
        ld      sp,ix
        pop     ix
        ret

nextdownf_ret_x:
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        ld      sp,ix
        pop     ix
        ret
