	.module xcc_output


	.area _CODE

	.globl _foo
_foo:
	; prologue: foo (locals=202)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-202
	add	hl, sp
	ld	sp, hl
	; receive param x at 4(ix)
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-204(ix), l
	ld	-203(ix), h
	ld	l, -200(ix)
	ld	h, -199(ix)
	ld	e, -204(ix)
	ld	d, -203(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-206(ix), l
	ld	-205(ix), h
	ld	l, -206(ix)
	ld	h, -205(ix)
	push	hl
	ld	de, #2000
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-208(ix), l
	ld	-207(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -208(ix)
	ld	d, -207(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-210(ix), l
	ld	-209(ix), h
	ld	l, -210(ix)
	ld	h, -209(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-212(ix), l
	ld	-211(ix), h
	ld	l, -212(ix)
	ld	h, -211(ix)
	push	hl
	ld	hl, #1000
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-214(ix), l
	ld	-213(ix), h
	ld	l, -214(ix)
	ld	h, -213(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L2
__xcc_L0:
	ld	hl, #1
	jp	__foo_end
	jp	__xcc_L2
__xcc_L2:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	-202(ix), l
	ld	-201(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-216(ix), l
	ld	-215(ix), h
	ld	l, -202(ix)
	ld	h, -201(ix)
	ld	e, -216(ix)
	ld	d, -215(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-218(ix), l
	ld	-217(ix), h
	ld	l, -218(ix)
	ld	h, -217(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-220(ix), l
	ld	-219(ix), h
	ld	l, -220(ix)
	ld	h, -219(ix)
	push	hl
	ld	hl, #1000
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-222(ix), l
	ld	-221(ix), h
	ld	l, -222(ix)
	ld	h, -221(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L5
__xcc_L3:
	ld	hl, #2
	jp	__foo_end
	jp	__xcc_L5
__xcc_L5:
	ld	l, -200(ix)
	ld	h, -199(ix)
	ld	-202(ix), l
	ld	-201(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-224(ix), l
	ld	-223(ix), h
	ld	l, -202(ix)
	ld	h, -201(ix)
	ld	e, -224(ix)
	ld	d, -223(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-226(ix), l
	ld	-225(ix), h
	ld	l, -226(ix)
	ld	h, -225(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-228(ix), l
	ld	-227(ix), h
	ld	l, -228(ix)
	ld	h, -227(ix)
	push	hl
	ld	hl, #2000
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_47793
	ld	hl, #0
	jp	__cmp_e_38335
__cmp_t_47793:
	ld	hl, #1
__cmp_e_38335:
	dec	sp
	dec	sp
	ld	-230(ix), l
	ld	-229(ix), h
	ld	l, -230(ix)
	ld	h, -229(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L8
__xcc_L6:
	ld	hl, #3
	jp	__foo_end
	jp	__xcc_L8
__xcc_L8:
	ld	hl, #200
	push	hl
	ld	hl, #2
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_85386
	ld	hl, #0
	jp	__cmp_e_60492
__cmp_t_85386:
	ld	hl, #1
__cmp_e_60492:
	dec	sp
	dec	sp
	ld	-232(ix), l
	ld	-231(ix), h
	ld	l, -232(ix)
	ld	h, -231(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L9
	jp	__xcc_L11
__xcc_L9:
	ld	hl, #4
	jp	__foo_end
	jp	__xcc_L11
__xcc_L11:
	ld	hl, #200
	push	hl
	ld	hl, #200
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_16649
	jp	m, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-234(ix), l
	ld	-233(ix), h
	ld	l, -234(ix)
	ld	h, -233(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L12
	jp	__xcc_L14
__xcc_L12:
	ld	hl, #5
	jp	__foo_end
	jp	__xcc_L14
__xcc_L14:
	ld	hl, #0
	jp	__foo_end
__foo_end:
	; epilogue: foo
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=200)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-200
	add	hl, sp
	ld	sp, hl
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #0
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-202(ix), l
	ld	-201(ix), h
	ld	l, -200(ix)
	ld	h, -199(ix)
	ld	e, -202(ix)
	ld	d, -201(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-204(ix), l
	ld	-203(ix), h
	ld	l, -204(ix)
	ld	h, -203(ix)
	push	hl
	ld	de, #1000
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -200(ix)
	ld	h, -199(ix)
	push	hl
	.globl _foo
	call	_foo
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
	ld	-206(ix), l
	ld	-205(ix), h
	ld	l, -206(ix)
	ld	h, -205(ix)
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
