        ;; tanf.s
        ;; Split from trigf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module tanf
        .optsdcc -mz80 sdcccall(1)

        .globl  _tanf
        .globl  ___fsdiv
        .globl  ___libc_fpclassifyf
        .globl  __tg_cospoly
        .globl  __tg_leave
        .globl  __tg_neg_hlde
        .globl  __tg_reduce
        .globl  __tg_ret_nan
        .globl  __tg_ret_x
        .globl  __tg_sinpoly

TG_CHI   .equ -2
TG_CLO   .equ -4
TG_QILO  .equ -24
TG_SHI   .equ -6
TG_SLO   .equ -8
TG_XHI   .equ -30
TG_XLO   .equ -32

        .area   _CODE
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
