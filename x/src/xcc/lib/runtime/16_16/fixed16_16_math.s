        ; fixed16_16_math.s
        ;
        ; Fixed 16.16 implementations for decomposition, scaling, remainder,
        ; and composed arithmetic float math entry points.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_math
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_ldexp
        .globl  _fixed16_16_scalbn
        .globl  _fixed16_16_scalbln
        .globl  _fixed16_16_ilogb
        .globl  _fixed16_16_logb
        .globl  _fixed16_16_significand
        .globl  _fixed16_16_frexp
        .globl  _fixed16_16_modf
        .globl  _fixed16_16_fma
        .globl  _fixed16_16_hypot
        .globl  _fixed16_16_fmod
        .globl  _fixed16_16_remainder
        .globl  _fixed16_16_remquo

        .globl  _fixed16_16_abs
        .globl  _fixed16_16_add
        .globl  _fixed16_16_div
        .globl  _fixed16_16_from_int
        .globl  _fixed16_16_mul
        .globl  _fixed16_16_round
        .globl  _fixed16_16_sqrt
        .globl  _fixed16_16_sub
        .globl  _fixed16_16_to_int
        .globl  _fixed16_16_trunc

X0      .equ    -16
X1      .equ    -15
X2      .equ    -14
X3      .equ    -13
Y0      .equ    -12
Y1      .equ    -11
Y2      .equ    -10
Y3      .equ    -9
T0      .equ    -8
T1      .equ    -7
T2      .equ    -6
T3      .equ    -5
Q0      .equ    -4
Q1      .equ    -3
Q2      .equ    -2
Q3      .equ    -1

        .area   _CODE

_fixed16_16_scalbn::
_fixed16_16_scalbln::
_fixed16_16_ldexp::
        push    ix
        ld      ix,#0
        add     ix,sp
        bit     7,5(ix)
        jr      nz,.ldexp_right
        ld      b,4(ix)
        ld      a,b
        or      a
        jr      z,.ldexp_done
.ldexp_left_loop:
        sla     e
        rl      d
        rl      l
        rl      h
        djnz    .ldexp_left_loop
        jr      .ldexp_done
.ldexp_right:
        xor     a
        sub     4(ix)
        ld      b,a
        or      a
        jr      z,.ldexp_done
.ldexp_right_loop:
        sra     h
        rr      l
        rr      d
        rr      e
        djnz    .ldexp_right_loop
.ldexp_done:
        pop     ix
        ret

_fixed16_16_ilogb::
        call    _fixed16_16_abs
        ld      a,e
        or      d
        or      l
        or      h
        jr      nz,.ilogb_nonzero
        ld      de,#0x8000
        ret
.ilogb_nonzero:
        ld      b,#0
.ilogb_loop:
        inc     b
        srl     h
        rr      l
        rr      d
        rr      e
        ld      a,e
        or      d
        or      l
        or      h
        jr      nz,.ilogb_loop
        ld      a,b
        sub     #17                     ; bit_index - 16 fractional bits
        ld      e,a
        rlca
        sbc     a,a
        ld      d,a
        ret

_fixed16_16_logb::
        call    _fixed16_16_ilogb
        ld      a,d
        cp      #0x80
        jr      nz,.logb_convert
        ld      de,#0
        ld      hl,#0x8000
        ret
.logb_convert:
        push    de
        pop     hl
        jp      _fixed16_16_from_int

_fixed16_16_significand::
        ld      a,e
        or      d
        or      l
        or      h
        jr      nz,.significand_nonzero
        ld      de,#0
        ld      hl,#0
        ret
.significand_nonzero:
        push    hl
        push    de
        call    _fixed16_16_ilogb
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        push    de
        pop     bc
        pop     de
        pop     hl
        push    bc
        call    _fixed16_16_ldexp
        pop     bc
        ret

_fixed16_16_frexp::
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
        ld      a,e
        or      d
        or      l
        or      h
        jr      nz,.frexp_nonzero
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,.frexp_zero_done
        xor     a
        ld      (bc),a
        inc     bc
        ld      (bc),a
.frexp_zero_done:
        ld      de,#0
        ld      hl,#0
        jp      .done
.frexp_nonzero:
        call    _fixed16_16_ilogb
        inc     de
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,.frexp_store_done
        ld      a,e
        ld      (bc),a
        inc     bc
        ld      a,d
        ld      (bc),a
.frexp_store_done:
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        push    de
        ld      e,X0(ix)
        ld      d,X1(ix)
        ld      l,X2(ix)
        ld      h,X3(ix)
        call    _fixed16_16_ldexp
        pop     bc
        jp      .done

_fixed16_16_modf::
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
        call    _fixed16_16_trunc
        ld      T0(ix),e
        ld      T1(ix),d
        ld      T2(ix),l
        ld      T3(ix),h
        ld      c,4(ix)
        ld      b,5(ix)
        ld      a,b
        or      c
        jr      z,.modf_store_done
        ld      a,T0(ix)
        ld      (bc),a
        inc     bc
        ld      a,T1(ix)
        ld      (bc),a
        inc     bc
        ld      a,T2(ix)
        ld      (bc),a
        inc     bc
        ld      a,T3(ix)
        ld      (bc),a
.modf_store_done:
        ld      l,T2(ix)
        ld      h,T3(ix)
        push    hl
        ld      l,T0(ix)
        ld      h,T1(ix)
        push    hl
        ld      e,X0(ix)
        ld      d,X1(ix)
        ld      l,X2(ix)
        ld      h,X3(ix)
        call    _fixed16_16_sub
        pop     bc
        pop     bc
        jp      .done

_fixed16_16_fma::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        call    _fixed16_16_mul
        pop     bc
        pop     bc
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        call    _fixed16_16_add
        pop     bc
        pop     bc
        pop     ix
        ret

_fixed16_16_hypot::
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
        call    .save_xy
        ld      l,X2(ix)
        ld      h,X3(ix)
        push    hl
        ld      l,X0(ix)
        ld      h,X1(ix)
        push    hl
        ld      e,X0(ix)
        ld      d,X1(ix)
        ld      l,X2(ix)
        ld      h,X3(ix)
        call    _fixed16_16_mul
        pop     bc
        pop     bc
        ld      T0(ix),e
        ld      T1(ix),d
        ld      T2(ix),l
        ld      T3(ix),h
        ld      l,Y2(ix)
        ld      h,Y3(ix)
        push    hl
        ld      l,Y0(ix)
        ld      h,Y1(ix)
        push    hl
        ld      e,Y0(ix)
        ld      d,Y1(ix)
        ld      l,Y2(ix)
        ld      h,Y3(ix)
        call    _fixed16_16_mul
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      e,T0(ix)
        ld      d,T1(ix)
        ld      l,T2(ix)
        ld      h,T3(ix)
        call    _fixed16_16_add
        pop     bc
        pop     bc
        call    _fixed16_16_sqrt
        jp      .done

_fixed16_16_fmod::
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
        call    .save_xy
        call    .quotient_trunc_frame
        jp      .finish_remainder

_fixed16_16_remainder::
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
        call    .save_xy
        call    .quotient_round_frame
        jp      .finish_remainder

_fixed16_16_remquo::
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
        call    .save_xy
        call    .quotient_round_frame
        ld      Q0(ix),e
        ld      Q1(ix),d
        ld      Q2(ix),l
        ld      Q3(ix),h
        call    _fixed16_16_to_int
        ld      c,8(ix)
        ld      b,9(ix)
        ld      a,b
        or      c
        jr      z,.remquo_store_done
        ld      a,e
        ld      (bc),a
        inc     bc
        ld      a,d
        ld      (bc),a
.remquo_store_done:
        ld      e,Q0(ix)
        ld      d,Q1(ix)
        ld      l,Q2(ix)
        ld      h,Q3(ix)
        jp      .finish_remainder

.save_xy:
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
        ret

.quotient_trunc_frame:
        ld      a,Y0(ix)
        or      Y1(ix)
        or      Y2(ix)
        or      Y3(ix)
        jr      nz,.qt_nonzero
        ld      de,#0
        ld      hl,#0
        ret
.qt_nonzero:
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
        call    _fixed16_16_div
        pop     bc
        pop     bc
        jp      _fixed16_16_trunc

.quotient_round_frame:
        ld      a,Y0(ix)
        or      Y1(ix)
        or      Y2(ix)
        or      Y3(ix)
        jr      nz,.qr_nonzero
        ld      de,#0
        ld      hl,#0
        ret
.qr_nonzero:
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
        call    _fixed16_16_div
        pop     bc
        pop     bc
        jp      _fixed16_16_round

.finish_remainder:
        ld      Q0(ix),e
        ld      Q1(ix),d
        ld      Q2(ix),l
        ld      Q3(ix),h
        ld      l,Y2(ix)
        ld      h,Y3(ix)
        push    hl
        ld      l,Y0(ix)
        ld      h,Y1(ix)
        push    hl
        ld      e,Q0(ix)
        ld      d,Q1(ix)
        ld      l,Q2(ix)
        ld      h,Q3(ix)
        call    _fixed16_16_mul
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      e,X0(ix)
        ld      d,X1(ix)
        ld      l,X2(ix)
        ld      h,X3(ix)
        call    _fixed16_16_sub
        pop     bc
        pop     bc
        jp      .done

.done:
        ld      sp,ix
        pop     ix
        ret
