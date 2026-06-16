        ;; cosf.s
        ;; Split from trigf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module cosf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cosf
        .globl  ___libc_fpclassifyf
        .globl  __tg_cospoly
        .globl  __tg_leave
        .globl  __tg_neg_hlde
        .globl  __tg_reduce
        .globl  __tg_ret_nan
        .globl  __tg_sinpoly

TG_XHI   .equ -30
TG_XLO   .equ -32

        .area   _CODE
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
