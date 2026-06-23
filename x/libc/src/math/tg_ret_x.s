        ;; tg_ret_x.s
        ;; Split from trigf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module tg_ret_x
        .optsdcc -mz80 sdcccall(1)

        .globl  __tg_ret_x
        .globl  __tg_leave

TG_XHI   .equ -30
TG_XLO   .equ -32

        .area   _CODE
__tg_ret_x::
	ld      l,TG_XHI(ix)
	ld      h,TG_XHI+1(ix)
	ld      e,TG_XLO(ix)
	ld      d,TG_XLO+1(ix)
	jp      __tg_leave

