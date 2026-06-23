        ; fixed8_8_c23.s
        ;
        ; Fixed 8.8 implementations for C23 float math entry points whose
        ; semantics are exact or naturally fixed-point based.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_c23
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_llround
        .globl  _fixed8_8_llrint
        .globl  _fixed8_8_fromfp
        .globl  _fixed8_8_ufromfp
        .globl  _fixed8_8_fromfpx
        .globl  _fixed8_8_ufromfpx
        .globl  _fixed8_8_fmaximum
        .globl  _fixed8_8_fminimum
        .globl  _fixed8_8_fmaximum_mag
        .globl  _fixed8_8_fminimum_mag
        .globl  _fixed8_8_fmaximum_num
        .globl  _fixed8_8_fminimum_num
        .globl  _fixed8_8_fmaximum_mag_num
        .globl  _fixed8_8_fminimum_mag_num
        .globl  _fixed8_8_getpayload
        .globl  _fixed8_8_setpayload
        .globl  _fixed8_8_setpayloadsig
        .globl  _fixed8_8_totalorder
        .globl  _fixed8_8_totalordermag
        .globl  _fixed8_8_nextafter
        .globl  _fixed8_8_nextup
        .globl  _fixed8_8_nextdown
        .globl  _fixed8_8_nan

        .globl  _fixed8_8_abs
        .globl  _fixed8_8_cmp
        .globl  _fixed8_8_fmax
        .globl  _fixed8_8_fmin
        .globl  _fixed8_8_lround
        .globl  _fixed8_8_round

        .area   _CODE

_fixed8_8_llrint::
_fixed8_8_llround::
        call    _fixed8_8_lround        ; DE:HL = signed 32-bit result
        ld      a,h
        rlca
        sbc     a,a
        exx
        ld      d,a
        ld      e,a
        ld      h,a
        ld      l,a
        exx
        ret

_fixed8_8_ufromfp::
_fixed8_8_fromfp::
_fixed8_8_ufromfpx::
_fixed8_8_fromfpx::
        jp      _fixed8_8_round

_fixed8_8_fmaximum_num::
_fixed8_8_fmaximum::
        jp      _fixed8_8_fmax

_fixed8_8_fminimum_num::
_fixed8_8_fminimum::
        jp      _fixed8_8_fmin

_fixed8_8_fmaximum_mag_num::
_fixed8_8_fmaximum_mag::
        call    .cmp_mag
        bit     7,b
        ret     nz                      ; |x| < |y|: y already in DE
        ld      a,b
        or      c
        jp      z,_fixed8_8_fmax        ; tie: numeric max
        ex      de,hl                   ; |x| > |y|
        ret

_fixed8_8_fminimum_mag_num::
_fixed8_8_fminimum_mag::
        call    .cmp_mag
        bit     7,b
        jr      nz,.return_x            ; |x| < |y|
        ld      a,b
        or      c
        jp      z,_fixed8_8_fmin        ; tie: numeric min
        ret                             ; |x| > |y|: y already in DE
.return_x:
        ex      de,hl
        ret

        ; Compare |x| and |y|.
        ; inputs:  HL = x, DE = y
        ; outputs: HL = x, DE = y, BC = cmp(|x|, |y|)
.cmp_mag:
        push    hl
        push    de
        call    _fixed8_8_abs
        ld      b,d
        ld      c,e
        pop     hl
        push    hl
        call    _fixed8_8_abs
        ld      h,b
        ld      l,c
        call    _fixed8_8_cmp
        ld      b,d
        ld      c,e
        pop     de
        pop     hl
        ret

_fixed8_8_getpayload::
_fixed8_8_nan::
        ld      de,#0
        ret

_fixed8_8_setpayloadsig::
_fixed8_8_setpayload::
        ld      de,#1                   ; fixed formats have no NaN payloads
        ret

_fixed8_8_totalorder::
        call    _fixed8_8_cmp
        bit     7,d
        jr      nz,.true
        ld      a,d
        or      e
        jr      z,.true
        ld      de,#0
        ret
.true:
        ld      de,#1
        ret

_fixed8_8_totalordermag::
        call    .cmp_mag
        bit     7,b
        jr      nz,.mag_true
        ld      a,b
        or      c
        jr      z,.mag_true
        ld      de,#0
        ret
.mag_true:
        ld      de,#1
        ret

_fixed8_8_nextafter::
        push    hl
        push    de
        call    _fixed8_8_cmp
        ld      b,d
        ld      c,e
        pop     de
        pop     hl
        ld      a,b
        or      c
        jr      z,.nextafter_y
        bit     7,b
        jr      nz,.nextafter_inc
        dec     hl
        ex      de,hl
        ret
.nextafter_inc:
        inc     hl
        ex      de,hl
        ret
.nextafter_y:
        ret

_fixed8_8_nextup::
        inc     hl
        ex      de,hl
        ret

_fixed8_8_nextdown::
        dec     hl
        ex      de,hl
        ret
