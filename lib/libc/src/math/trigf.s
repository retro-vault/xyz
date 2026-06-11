        ;; trigf.s
        ;;
        ;; libc sinf / cosf / tanf for the xcc Z80 libc.
        ;; Range reduction uses q = round(x * 2/pi), then evaluates small-angle
        ;; Taylor polynomials on r = x - q*(pi/2), where |r| <= pi/4.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module trigf
        .optsdcc -mz80 sdcccall(1)

        .globl  _sinf
        .globl  _cosf
        .globl  _tanf
        .globl  ___libc_fpclassifyf
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  ___fssub
	.globl  ___fsdiv
	.globl  _roundf
	.globl  ___fs2slong

TG_XLO   .equ -32
TG_XHI   .equ -30
TG_QFLO  .equ -28
TG_QFHI  .equ -26
TG_QILO  .equ -24
TG_QIHI  .equ -22
TG_RLO   .equ -20
TG_RHI   .equ -18
TG_X2LO  .equ -16
TG_X2HI  .equ -14
TG_ULO   .equ -12
TG_UHI   .equ -10
TG_SLO   .equ -8
TG_SHI   .equ -6
TG_CLO   .equ -4
TG_CHI   .equ -2

	.area   _CODE

__tg_ret_x:
	ld      l,TG_XHI(ix)
	ld      h,TG_XHI+1(ix)
	ld      e,TG_XLO(ix)
	ld      d,TG_XLO+1(ix)
	jp      __tg_leave

__tg_ret_nan:
	ld      hl,#0x7fc0
	ld      de,#0x0000
	jp      __tg_leave

__tg_neg_hlde:
	ld      a,h
	xor     #0x80
	ld      h,a
	jp      __tg_leave

__tg_leave:
	ld      sp,ix
	pop     ix
	ret

        ;; __tg_reduce
        ;; input:  __tg_x already filled
        ;; output: __tg_qf, __tg_qi, __tg_r
        ;;         A = quadrant = q mod 4
__tg_reduce:
	;; qf = roundf(x * 2/pi)
	ld      hl,#0x3f22              ; 2/pi
	push    hl
	ld      hl,#0xf983
	push    hl
	ld      e,TG_XLO(ix)
	ld      d,TG_XLO+1(ix)
	ld      l,TG_XHI(ix)
	ld      h,TG_XHI+1(ix)
	call    ___fsmul
	pop     bc
	pop     bc
	call    _roundf
	ld      TG_QFLO(ix),e
	ld      TG_QFLO+1(ix),d
	ld      TG_QFHI(ix),l
	ld      TG_QFHI+1(ix),h

	;; qi = (long)qf
	call    ___fs2slong
	ld      TG_QILO(ix),e
	ld      TG_QILO+1(ix),d
	ld      TG_QIHI(ix),l
	ld      TG_QIHI+1(ix),h

	;; r = x - qf * (pi/2)
	ld      hl,#0x3fc9              ; pi/2
	push    hl
	ld      hl,#0x0fdb
	push    hl
	ld      e,TG_QFLO(ix)
	ld      d,TG_QFLO+1(ix)
	ld      l,TG_QFHI(ix)
	ld      h,TG_QFHI+1(ix)
	call    ___fsmul
	pop     bc
	pop     bc
	ld      TG_SLO(ix),e            ; temporary product slot
	ld      TG_SLO+1(ix),d
	ld      TG_SHI(ix),l
	ld      TG_SHI+1(ix),h
	ld      l,TG_SHI(ix)
	ld      h,TG_SHI+1(ix)
	push    hl
	ld      l,TG_SLO(ix)
	ld      h,TG_SLO+1(ix)
	push    hl
	ld      e,TG_XLO(ix)
	ld      d,TG_XLO+1(ix)
	ld      l,TG_XHI(ix)
	ld      h,TG_XHI+1(ix)
	call    ___fssub
	pop     bc
	pop     bc
	ld      TG_RLO(ix),e
	ld      TG_RLO+1(ix),d
	ld      TG_RHI(ix),l
	ld      TG_RHI+1(ix),h

	ld      a,TG_QILO(ix)
	and     #3
	ret

        ;; __tg_square_r
        ;; computes __tg_x2 = __tg_r * __tg_r
__tg_square_r:
	ld      l,TG_RHI(ix)
	ld      h,TG_RHI+1(ix)
	push    hl
	ld      l,TG_RLO(ix)
	ld      h,TG_RLO+1(ix)
	push    hl
	ld      e,TG_RLO(ix)
	ld      d,TG_RLO+1(ix)
	ld      l,TG_RHI(ix)
	ld      h,TG_RHI+1(ix)
	call    ___fsmul
	pop     bc
	pop     bc
	ld      TG_X2LO(ix),e
	ld      TG_X2LO+1(ix),d
	ld      TG_X2HI(ix),l
	ld      TG_X2HI+1(ix),h
	ret

        ;; __tg_sinpoly
        ;; returns sin(__tg_r) in HL:DE for |r| <= pi/4
__tg_sinpoly:
        call    __tg_square_r

        ;; u = c7*x2 + c5
	ld      hl,#0xb950              ; -1/5040
	push    hl
	ld      hl,#0x0d01
	push    hl
	ld      e,TG_X2LO(ix)
	ld      d,TG_X2LO+1(ix)
	ld      l,TG_X2HI(ix)
	ld      h,TG_X2HI+1(ix)
	call    ___fsmul
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h
	ld      hl,#0x3c08              ; +1/120
	push    hl
	ld      hl,#0x8889
	push    hl
	ld      e,TG_ULO(ix)
	ld      d,TG_ULO+1(ix)
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	call    ___fsadd
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h

	;; u = u*x2 + c3
	ld      l,TG_X2HI(ix)
	ld      h,TG_X2HI+1(ix)
	push    hl
	ld      l,TG_X2LO(ix)
	ld      h,TG_X2LO+1(ix)
	push    hl
	ld      e,TG_ULO(ix)
	ld      d,TG_ULO+1(ix)
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	call    ___fsmul
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h
	ld      hl,#0xbe2a              ; -1/6
	push    hl
	ld      hl,#0xaaab
	push    hl
	ld      e,TG_ULO(ix)
	ld      d,TG_ULO+1(ix)
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	call    ___fsadd
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h

	;; u = 1 + u*x2
	ld      l,TG_X2HI(ix)
	ld      h,TG_X2HI+1(ix)
	push    hl
	ld      l,TG_X2LO(ix)
	ld      h,TG_X2LO+1(ix)
	push    hl
	ld      e,TG_ULO(ix)
	ld      d,TG_ULO+1(ix)
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	call    ___fsmul
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h
	ld      hl,#0x3f80              ; +1.0
	push    hl
	ld      hl,#0x0000
	push    hl
	ld      e,TG_ULO(ix)
	ld      d,TG_ULO+1(ix)
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	call    ___fsadd
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h

	;; result = r * u
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	push    hl
	ld      l,TG_ULO(ix)
	ld      h,TG_ULO+1(ix)
	push    hl
	ld      e,TG_RLO(ix)
	ld      d,TG_RLO+1(ix)
	ld      l,TG_RHI(ix)
	ld      h,TG_RHI+1(ix)
	call    ___fsmul
	pop     bc
	pop     bc
        ret

        ;; __tg_cospoly
        ;; returns cos(__tg_r) in HL:DE for |r| <= pi/4
__tg_cospoly:
        call    __tg_square_r

        ;; u = c6*x2 + c4
	ld      hl,#0xbab6              ; -1/720
	push    hl
	ld      hl,#0x0b61
	push    hl
	ld      e,TG_X2LO(ix)
	ld      d,TG_X2LO+1(ix)
	ld      l,TG_X2HI(ix)
	ld      h,TG_X2HI+1(ix)
	call    ___fsmul
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h
	ld      hl,#0x3d2a              ; +1/24
	push    hl
	ld      hl,#0xaaab
	push    hl
	ld      e,TG_ULO(ix)
	ld      d,TG_ULO+1(ix)
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	call    ___fsadd
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h

	;; u = u*x2 + c2
	ld      l,TG_X2HI(ix)
	ld      h,TG_X2HI+1(ix)
	push    hl
	ld      l,TG_X2LO(ix)
	ld      h,TG_X2LO+1(ix)
	push    hl
	ld      e,TG_ULO(ix)
	ld      d,TG_ULO+1(ix)
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	call    ___fsmul
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h
	ld      hl,#0xbf00              ; -1/2
	push    hl
	ld      hl,#0x0000
	push    hl
	ld      e,TG_ULO(ix)
	ld      d,TG_ULO+1(ix)
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	call    ___fsadd
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h

	;; result = 1 + u*x2
	ld      l,TG_X2HI(ix)
	ld      h,TG_X2HI+1(ix)
	push    hl
	ld      l,TG_X2LO(ix)
	ld      h,TG_X2LO+1(ix)
	push    hl
	ld      e,TG_ULO(ix)
	ld      d,TG_ULO+1(ix)
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	call    ___fsmul
	pop     bc
	pop     bc
	ld      TG_ULO(ix),e
	ld      TG_ULO+1(ix),d
	ld      TG_UHI(ix),l
	ld      TG_UHI+1(ix),h
	ld      hl,#0x3f80              ; +1.0
	push    hl
	ld      hl,#0x0000
	push    hl
	ld      e,TG_ULO(ix)
	ld      d,TG_ULO+1(ix)
	ld      l,TG_UHI(ix)
	ld      h,TG_UHI+1(ix)
	call    ___fsadd
	pop     bc
	pop     bc
	ret

        ;; HL:DE = x -> HL:DE = sin(x)
_sinf::
	push    ix
	ld      ix,#0
	add     ix,sp
	ld      c,l
	ld      b,h
	ld      hl,#-32
	add     hl,sp
	ld      sp,hl
	ld      TG_XLO(ix),e
	ld      TG_XLO+1(ix),d
	ld      TG_XHI(ix),c
	ld      TG_XHI+1(ix),b
	call    ___libc_fpclassifyf
	ld      a,e
	cp      #2
	jp      c,__tg_ret_nan
	jp      z,__tg_ret_x
	ld      e,TG_XLO(ix)
	ld      d,TG_XLO+1(ix)
	ld      l,TG_XHI(ix)
	ld      h,TG_XHI+1(ix)
	call    __tg_reduce
	cp      #1
	jr      z,tg_sin_q1
	cp      #2
	jr      z,tg_sin_q2
	cp      #3
	jr      z,tg_sin_q3
	call    __tg_sinpoly
	jp      __tg_leave
tg_sin_q1:
	call    __tg_cospoly
	jp      __tg_leave
tg_sin_q2:
	call    __tg_sinpoly
	jp      __tg_neg_hlde
tg_sin_q3:
        call    __tg_cospoly
        jp      __tg_neg_hlde

        ;; HL:DE = x -> HL:DE = cos(x)
_cosf::
	push    ix
	ld      ix,#0
	add     ix,sp
	ld      c,l
	ld      b,h
	ld      hl,#-32
	add     hl,sp
	ld      sp,hl
	ld      TG_XLO(ix),e
	ld      TG_XLO+1(ix),d
	ld      TG_XHI(ix),c
	ld      TG_XHI+1(ix),b
	call    ___libc_fpclassifyf
	ld      a,e
	cp      #2
	jp      c,__tg_ret_nan
	jr      z,tg_cos_zero
	ld      e,TG_XLO(ix)
	ld      d,TG_XLO+1(ix)
	ld      l,TG_XHI(ix)
	ld      h,TG_XHI+1(ix)
	call    __tg_reduce
	cp      #1
	jr      z,tg_cos_q1
	cp      #2
	jr      z,tg_cos_q2
	cp      #3
	jr      z,tg_cos_q3
	call    __tg_cospoly
	jp      __tg_leave
tg_cos_q1:
	call    __tg_sinpoly
	jp      __tg_neg_hlde
tg_cos_q2:
        call    __tg_cospoly
        jp      __tg_neg_hlde
tg_cos_q3:
	call    __tg_sinpoly
	jp      __tg_leave
tg_cos_zero:
	ld      hl,#0x3f80
	ld      de,#0x0000
	jp      __tg_leave

        ;; HL:DE = x -> HL:DE = tan(x)
_tanf::
	push    ix
	ld      ix,#0
	add     ix,sp
	ld      c,l
	ld      b,h
	ld      hl,#-32
	add     hl,sp
	ld      sp,hl
	ld      TG_XLO(ix),e
	ld      TG_XLO+1(ix),d
	ld      TG_XHI(ix),c
	ld      TG_XHI+1(ix),b
	call    ___libc_fpclassifyf
	ld      a,e
	cp      #2
	jp      c,__tg_ret_nan
	jp      z,__tg_ret_x
	ld      e,TG_XLO(ix)
	ld      d,TG_XLO+1(ix)
	ld      l,TG_XHI(ix)
	ld      h,TG_XHI+1(ix)
	call    __tg_reduce
	call    __tg_sinpoly
	ld      TG_SLO(ix),e
	ld      TG_SLO+1(ix),d
	ld      TG_SHI(ix),l
	ld      TG_SHI+1(ix),h
	call    __tg_cospoly
	ld      TG_CLO(ix),e
	ld      TG_CLO+1(ix),d
	ld      TG_CHI(ix),l
	ld      TG_CHI+1(ix),h
	ld      a,TG_QILO(ix)
	and     #1
	jr      nz,tg_tan_odd
	ld      l,TG_CHI(ix)
	ld      h,TG_CHI+1(ix)
	push    hl
	ld      l,TG_CLO(ix)
	ld      h,TG_CLO+1(ix)
	push    hl
	ld      e,TG_SLO(ix)
	ld      d,TG_SLO+1(ix)
	ld      l,TG_SHI(ix)
	ld      h,TG_SHI+1(ix)
	call    ___fsdiv
	pop     bc
	pop     bc
	jp      __tg_leave
tg_tan_odd:
	ld      l,TG_SHI(ix)
	ld      h,TG_SHI+1(ix)
	push    hl
	ld      l,TG_SLO(ix)
	ld      h,TG_SLO+1(ix)
	push    hl
	ld      e,TG_CLO(ix)
	ld      d,TG_CLO+1(ix)
	ld      l,TG_CHI(ix)
	ld      h,TG_CHI+1(ix)
	call    ___fsdiv
	pop     bc
	pop     bc
	jp      __tg_neg_hlde
