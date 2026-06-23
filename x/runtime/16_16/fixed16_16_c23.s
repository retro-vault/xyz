        ; fixed16_16_c23.s
        ;
        ; Fixed 16.16 implementations for C23 float math entry points whose
        ; semantics are exact or naturally fixed-point based.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_c23
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_llround
        .globl  _fixed16_16_llrint
        .globl  _fixed16_16_fromfp
        .globl  _fixed16_16_ufromfp
        .globl  _fixed16_16_fromfpx
        .globl  _fixed16_16_ufromfpx
        .globl  _fixed16_16_fmaximum
        .globl  _fixed16_16_fminimum
        .globl  _fixed16_16_fmaximum_mag
        .globl  _fixed16_16_fminimum_mag
        .globl  _fixed16_16_fmaximum_num
        .globl  _fixed16_16_fminimum_num
        .globl  _fixed16_16_fmaximum_mag_num
        .globl  _fixed16_16_fminimum_mag_num
        .globl  _fixed16_16_getpayload
        .globl  _fixed16_16_setpayload
        .globl  _fixed16_16_setpayloadsig
        .globl  _fixed16_16_totalorder
        .globl  _fixed16_16_totalordermag
        .globl  _fixed16_16_nextafter
        .globl  _fixed16_16_nextup
        .globl  _fixed16_16_nextdown
        .globl  _fixed16_16_nan

        .globl  _fixed16_16_abs
        .globl  _fixed16_16_cmp
        .globl  _fixed16_16_fmax
        .globl  _fixed16_16_fmin
        .globl  _fixed16_16_lround
        .globl  _fixed16_16_round

X0      .equ    -16
X1      .equ    -15
X2      .equ    -14
X3      .equ    -13
Y0      .equ    -12
Y1      .equ    -11
Y2      .equ    -10
Y3      .equ    -9
AX0     .equ    -8
AX1     .equ    -7
AX2     .equ    -6
AX3     .equ    -5
AY0     .equ    -4
AY1     .equ    -3
AY2     .equ    -2
AY3     .equ    -1

        .area   _CODE

_fixed16_16_llrint::
_fixed16_16_llround::
        call    _fixed16_16_lround      ; DE:HL = signed 32-bit result
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

_fixed16_16_ufromfp::
_fixed16_16_fromfp::
_fixed16_16_ufromfpx::
_fixed16_16_fromfpx::
        jp      _fixed16_16_round

_fixed16_16_fmaximum_num::
_fixed16_16_fmaximum::
        jp      _fixed16_16_fmax

_fixed16_16_fminimum_num::
_fixed16_16_fminimum::
        jp      _fixed16_16_fmin

_fixed16_16_fmaximum_mag_num::
_fixed16_16_fmaximum_mag::
        call    .cmp_mag
        bit     7,b
        jr      nz,.load_y_done         ; |x| < |y|
        ld      a,b
        or      c
        jr      z,.tie_max
        jr      .load_x_done
.tie_max:
        ld      l,Y2(ix)
        ld      h,Y3(ix)
        push    hl
        ld      l,Y0(ix)
        ld      h,Y1(ix)
        push    hl
        ld      e,X0(ix)
        ld      d,X1(ix)
        ld      l,X2(ix)
        ld      h,X3(ix)
        call    _fixed16_16_fmax
        pop     bc
        pop     bc
        jr      .done

_fixed16_16_fminimum_mag_num::
_fixed16_16_fminimum_mag::
        call    .cmp_mag
        bit     7,b
        jr      nz,.load_x_done         ; |x| < |y|
        ld      a,b
        or      c
        jr      z,.tie_min
        jr      .load_y_done
.tie_min:
        ld      l,Y2(ix)
        ld      h,Y3(ix)
        push    hl
        ld      l,Y0(ix)
        ld      h,Y1(ix)
        push    hl
        ld      e,X0(ix)
        ld      d,X1(ix)
        ld      l,X2(ix)
        ld      h,X3(ix)
        call    _fixed16_16_fmin
        pop     bc
        pop     bc
        jr      .done
.load_x_done:
        ld      e,X0(ix)
        ld      d,X1(ix)
        ld      l,X2(ix)
        ld      h,X3(ix)
        jr      .done
.load_y_done:
        ld      e,Y0(ix)
        ld      d,Y1(ix)
        ld      l,Y2(ix)
        ld      h,Y3(ix)
.done:
        ld      sp,ix
        pop     ix
        ret

        ; Compare |x| and |y|.
        ; outputs: IX frame with saved x/y, BC = cmp(|x|, |y|)
.cmp_mag:
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      h,b
        ld      l,c
        ld      X0(ix),e
        ld      X1(ix),d
        ld      X2(ix),l
        ld      X3(ix),h
        ld      a,4(ix)
        ld      Y0(ix),a
        ld      a,5(ix)
        ld      Y1(ix),a
        ld      a,6(ix)
        ld      Y2(ix),a
        ld      a,7(ix)
        ld      Y3(ix),a

        call    _fixed16_16_abs
        ld      AX0(ix),e
        ld      AX1(ix),d
        ld      AX2(ix),l
        ld      AX3(ix),h
        ld      e,Y0(ix)
        ld      d,Y1(ix)
        ld      l,Y2(ix)
        ld      h,Y3(ix)
        call    _fixed16_16_abs
        push    hl
        push    de
        ld      e,AX0(ix)
        ld      d,AX1(ix)
        ld      l,AX2(ix)
        ld      h,AX3(ix)
        call    _fixed16_16_cmp
        pop     bc
        pop     bc
        ld      b,d
        ld      c,e
        ret

_fixed16_16_getpayload::
_fixed16_16_nan::
        ld      de,#0
        ld      hl,#0
        ret

_fixed16_16_setpayloadsig::
_fixed16_16_setpayload::
        ld      de,#1
        ret

_fixed16_16_totalorder::
        call    _fixed16_16_cmp
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

_fixed16_16_totalordermag::
        call    .cmp_mag
        bit     7,b
        jr      nz,.mag_true
        ld      a,b
        or      c
        jr      z,.mag_true
        ld      de,#0
        jp      .done
.mag_true:
        ld      de,#1
        jp      .done

_fixed16_16_nextafter::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-16
        add     hl,sp
        ld      sp,hl
        ld      h,b
        ld      l,c
        ld      X0(ix),e
        ld      X1(ix),d
        ld      X2(ix),l
        ld      X3(ix),h
        ld      a,4(ix)
        ld      Y0(ix),a
        ld      a,5(ix)
        ld      Y1(ix),a
        ld      a,6(ix)
        ld      Y2(ix),a
        ld      a,7(ix)
        ld      Y3(ix),a
        call    _fixed16_16_cmp
        ld      b,d
        ld      c,e
        ld      e,X0(ix)
        ld      d,X1(ix)
        ld      l,X2(ix)
        ld      h,X3(ix)
        ld      a,b
        or      c
        jr      z,.nextafter_y
        bit     7,b
        jr      nz,.nextafter_inc
        dec     de
        ld      a,d
        and     e
        cp      #0xff
        jr      nz,.nextafter_dec_done
        dec     hl
.nextafter_dec_done:
        jp      .done
.nextafter_inc:
        inc     de
        ld      a,d
        or      e
        jp      nz,.done
        inc     hl
        jp      .done
.nextafter_y:
        ld      e,Y0(ix)
        ld      d,Y1(ix)
        ld      l,Y2(ix)
        ld      h,Y3(ix)
        jp      .done

_fixed16_16_nextup::
        inc     de
        ld      a,d
        or      e
        ret     nz
        inc     hl
        ret

_fixed16_16_nextdown::
        dec     de
        ld      a,d
        and     e
        cp      #0xff
        ret     nz
        dec     hl
        ret
