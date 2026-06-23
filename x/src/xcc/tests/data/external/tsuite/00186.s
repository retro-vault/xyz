	.module xcc_output

	.area _CONST
__str_4:
	.db 45, 62, 37, 48, 50, 100, 60, 45, 10, 0
__str_5:
	.db 37, 115, 0


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=102)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-102
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	-102(ix), l
	ld	-101(ix), h
__xcc_L0:
	ld	l, -102(ix)
	ld	h, -101(ix)
	push	hl
	ld	hl, #20
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_89383
	jp	m, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-104(ix), l
	ld	-103(ix), h
	ld	l, -104(ix)
	ld	h, -103(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L3
__xcc_L1:
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-106(ix), l
	ld	-105(ix), h
	ld	l, -102(ix)
	ld	h, -101(ix)
	push	hl
	ld	l, -106(ix)
	ld	h, -105(ix)
	push	hl
	ld	l, -100(ix)
	ld	h, -99(ix)
	push	hl
	.globl _sprintf
	call	_sprintf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-108(ix), l
	ld	-107(ix), h
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-110(ix), l
	ld	-109(ix), h
	ld	l, -100(ix)
	ld	h, -99(ix)
	push	hl
	ld	l, -110(ix)
	ld	h, -109(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-112(ix), l
	ld	-111(ix), h
__xcc_L2:
	ld	l, -102(ix)
	ld	h, -101(ix)
	dec	sp
	dec	sp
	ld	-114(ix), l
	ld	-113(ix), h
	ld	l, -102(ix)
	ld	h, -101(ix)
	inc	hl
	ld	-102(ix), l
	ld	-101(ix), h
	jp	__xcc_L0
__xcc_L3:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
