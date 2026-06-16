        ;; tg_ret_nan.s
        ;; Split from trigf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module tg_ret_nan
        .optsdcc -mz80 sdcccall(1)

        .globl  __tg_cospoly
        .globl  __tg_leave
        .globl  __tg_neg_hlde
        .globl  __tg_reduce
        .globl  __tg_ret_nan
        .globl  __tg_sinpoly
        .globl  ___fs2slong
        .globl  ___fsadd
        .globl  ___fsmul
        .globl  ___fssub
        .globl  _roundf

TG_QFHI  .equ -26
TG_QFLO  .equ -28
TG_QIHI  .equ -22
TG_QILO  .equ -24
TG_RHI   .equ -18
TG_RLO   .equ -20
TG_SHI   .equ -6
TG_SLO   .equ -8
TG_UHI   .equ -10
TG_ULO   .equ -12
TG_X2HI  .equ -14
TG_X2LO  .equ -16
TG_XHI   .equ -30
TG_XLO   .equ -32

        .area   _CODE
__tg_ret_nan::
	ld      hl,#0x7fc0
	ld      de,#0x0000
	jp      __tg_leave

__tg_neg_hlde::
	ld      a,h
	xor     #0x80
	ld      h,a
	jp      __tg_leave

__tg_leave::
	ld      sp,ix
	pop     ix
	ret

        ;; __tg_reduce
        ;; input:  __tg_x already filled
        ;; output: __tg_qf, __tg_qi, __tg_r
        ;;         A = quadrant = q mod 4
__tg_reduce::
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
__tg_sinpoly::
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
__tg_cospoly::
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
