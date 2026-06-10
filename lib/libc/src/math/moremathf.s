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

        .area   _DATA
__mf_x: .ds 4
__mf_y: .ds 4
__mf_q: .ds 4
__mf_t: .ds 4

        .area   _CODE

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
        ld      (__mf_x),de
        ld      (__mf_x + 2),hl
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
        ld      de,(__mf_x)
        ld      hl,(__mf_x + 2)
        call    _ldexpf
        pop     bc
        pop     ix
        ret

        ;; float fmaf(float x, float y, float z)
        ;; x in HL:DE, y at 4(ix)..7(ix), z at 8(ix)..11(ix)
_fmaf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__mf_x),de
        ld      (__mf_x + 2),hl
        ld      a,4(ix)
        ld      (__mf_y),a
        ld      a,5(ix)
        ld      (__mf_y + 1),a
        ld      a,6(ix)
        ld      (__mf_y + 2),a
        ld      a,7(ix)
        ld      (__mf_y + 3),a
        ld      hl,(__mf_y + 2)
        push    hl
        ld      hl,(__mf_y)
        push    hl
        ld      de,(__mf_x)
        ld      hl,(__mf_x + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__mf_t),de
        ld      (__mf_t + 2),hl
        ld      a,8(ix)
        ld      (__mf_y),a
        ld      a,9(ix)
        ld      (__mf_y + 1),a
        ld      a,10(ix)
        ld      (__mf_y + 2),a
        ld      a,11(ix)
        ld      (__mf_y + 3),a
        ld      hl,(__mf_y + 2)
        push    hl
        ld      hl,(__mf_y)
        push    hl
        ld      de,(__mf_t)
        ld      hl,(__mf_t + 2)
        call    ___fsadd
        pop     bc
        pop     bc
        pop     ix
        ret

        ;; float hypotf(float x, float y)
_hypotf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__mf_x),de
        ld      (__mf_x + 2),hl
        ld      a,4(ix)
        ld      (__mf_y),a
        ld      a,5(ix)
        ld      (__mf_y + 1),a
        ld      a,6(ix)
        ld      (__mf_y + 2),a
        ld      a,7(ix)
        ld      (__mf_y + 3),a
        ;; t = x * x
        ld      hl,(__mf_x + 2)
        push    hl
        ld      hl,(__mf_x)
        push    hl
        ld      de,(__mf_x)
        ld      hl,(__mf_x + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__mf_t),de
        ld      (__mf_t + 2),hl
        ;; x*x + y*y via fmaf(y, y, t)
        ld      hl,(__mf_t + 2)
        push    hl
        ld      hl,(__mf_t)
        push    hl
        ld      hl,(__mf_y + 2)
        push    hl
        ld      hl,(__mf_y)
        push    hl
        ld      de,(__mf_y)
        ld      hl,(__mf_y + 2)
        call    _fmaf
        pop     bc
        pop     bc
        pop     bc
        pop     bc
        call    _sqrtf
        pop     ix
        ret

        ;; float fmodf(float x, float y)
_fmodf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__mf_x),de
        ld      (__mf_x + 2),hl
        ld      a,4(ix)
        ld      (__mf_y),a
        ld      a,5(ix)
        ld      (__mf_y + 1),a
        ld      a,6(ix)
        ld      (__mf_y + 2),a
        ld      a,7(ix)
        ld      (__mf_y + 3),a
        ld      a,(__mf_y + 3)
        and     #0x7f
        ld      b,a
        ld      a,(__mf_y + 2)
        or      b
        ld      b,a
        ld      a,(__mf_y + 1)
        or      b
        ld      b,a
        ld      a,(__mf_y)
        or      b
        jr      nz,fmodf_div
        ld      hl,#0x7fc0
        ld      de,#0x0000
        pop     ix
        ret
fmodf_div:
        ld      hl,(__mf_y + 2)
        push    hl
        ld      hl,(__mf_y)
        push    hl
        ld      de,(__mf_x)
        ld      hl,(__mf_x + 2)
        call    ___fsdiv
        pop     bc
        pop     bc
        bit     7,h
        call    nz,_ceilf
        call    z,_floorf
        ld      (__mf_q),de
        ld      (__mf_q + 2),hl
        ld      hl,(__mf_y + 2)
        push    hl
        ld      hl,(__mf_y)
        push    hl
        ld      de,(__mf_q)
        ld      hl,(__mf_q + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__mf_t),de
        ld      (__mf_t + 2),hl
        ld      hl,(__mf_t + 2)
        push    hl
        ld      hl,(__mf_t)
        push    hl
        ld      de,(__mf_x)
        ld      hl,(__mf_x + 2)
        call    ___fssub
        pop     bc
        pop     bc
        pop     ix
        ret

        ;; float remainderf(float x, float y)
_remainderf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__mf_x),de
        ld      (__mf_x + 2),hl
        ld      a,4(ix)
        ld      (__mf_y),a
        ld      a,5(ix)
        ld      (__mf_y + 1),a
        ld      a,6(ix)
        ld      (__mf_y + 2),a
        ld      a,7(ix)
        ld      (__mf_y + 3),a
        ld      a,(__mf_y + 3)
        and     #0x7f
        ld      b,a
        ld      a,(__mf_y + 2)
        or      b
        ld      b,a
        ld      a,(__mf_y + 1)
        or      b
        ld      b,a
        ld      a,(__mf_y)
        or      b
        jr      nz,remainderf_div
        ld      hl,#0x7fc0
        ld      de,#0x0000
        pop     ix
        ret
remainderf_div:
        ld      hl,(__mf_y + 2)
        push    hl
        ld      hl,(__mf_y)
        push    hl
        ld      de,(__mf_x)
        ld      hl,(__mf_x + 2)
        call    ___fsdiv
        pop     bc
        pop     bc
        call    _roundf
        ld      (__mf_q),de
        ld      (__mf_q + 2),hl
        ld      hl,(__mf_y + 2)
        push    hl
        ld      hl,(__mf_y)
        push    hl
        ld      de,(__mf_q)
        ld      hl,(__mf_q + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__mf_t),de
        ld      (__mf_t + 2),hl
        ld      hl,(__mf_t + 2)
        push    hl
        ld      hl,(__mf_t)
        push    hl
        ld      de,(__mf_x)
        ld      hl,(__mf_x + 2)
        call    ___fssub
        pop     bc
        pop     bc
        pop     ix
        ret

        ;; float remquof(float x, float y, int *quo)
_remquof::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__mf_x),de
        ld      (__mf_x + 2),hl
        ld      a,4(ix)
        ld      (__mf_y),a
        ld      a,5(ix)
        ld      (__mf_y + 1),a
        ld      a,6(ix)
        ld      (__mf_y + 2),a
        ld      a,7(ix)
        ld      (__mf_y + 3),a
        ld      a,(__mf_y + 3)
        and     #0x7f
        ld      b,a
        ld      a,(__mf_y + 2)
        or      b
        ld      b,a
        ld      a,(__mf_y + 1)
        or      b
        ld      b,a
        ld      a,(__mf_y)
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
        pop     ix
        ret
remquof_div:
        ld      hl,(__mf_y + 2)
        push    hl
        ld      hl,(__mf_y)
        push    hl
        ld      de,(__mf_x)
        ld      hl,(__mf_x + 2)
        call    ___fsdiv
        pop     bc
        pop     bc
        call    _roundf
        ld      (__mf_q),de
        ld      (__mf_q + 2),hl
        ld      c,8(ix)
        ld      b,9(ix)
        ld      a,b
        or      c
        jr      z,remquof_store_done
        ld      de,(__mf_q)
        ld      hl,(__mf_q + 2)
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
        ld      hl,(__mf_y + 2)
        push    hl
        ld      hl,(__mf_y)
        push    hl
        ld      de,(__mf_q)
        ld      hl,(__mf_q + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__mf_t),de
        ld      (__mf_t + 2),hl
        ld      hl,(__mf_t + 2)
        push    hl
        ld      hl,(__mf_t)
        push    hl
        ld      de,(__mf_x)
        ld      hl,(__mf_x + 2)
        call    ___fssub
        pop     bc
        pop     bc
        pop     ix
        ret

        ;; float nextafterf(float x, float y)
_nextafterf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__mf_x),de
        ld      (__mf_x + 2),hl
        call    __float_cmp_xy
        ld      b,a
        or      a
        jp      z,nextafterf_ret_y
        ld      a,(__mf_x + 3)
        and     #0x7f
        ld      c,a
        ld      a,(__mf_x + 2)
        or      c
        ld      c,a
        ld      a,(__mf_x + 1)
        or      c
        ld      c,a
        ld      a,(__mf_x)
        or      c
        jr      nz,nextafterf_nonzero
        ld      a,7(ix)
        and     #0x80
        ld      h,a
        ld      l,#0x00
        ld      d,#0x00
        ld      e,#0x01
        pop     ix
        ret
nextafterf_nonzero:
        ld      a,(__mf_x + 3)
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
        ld      a,(__mf_x)
        add     a,#1
        ld      (__mf_x),a
        ld      a,(__mf_x + 1)
        adc     a,#0
        ld      (__mf_x + 1),a
        ld      a,(__mf_x + 2)
        adc     a,#0
        ld      (__mf_x + 2),a
        ld      a,(__mf_x + 3)
        adc     a,#0
        ld      (__mf_x + 3),a
        jr      nextafterf_ret_x
nextafterf_dec:
        ld      a,(__mf_x)
        sub     #1
        ld      (__mf_x),a
        ld      a,(__mf_x + 1)
        sbc     a,#0
        ld      (__mf_x + 1),a
        ld      a,(__mf_x + 2)
        sbc     a,#0
        ld      (__mf_x + 2),a
        ld      a,(__mf_x + 3)
        sbc     a,#0
        ld      (__mf_x + 3),a
nextafterf_ret_x:
        ld      de,(__mf_x)
        ld      hl,(__mf_x + 2)
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
        pop     ix
        ret
