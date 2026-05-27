	.module xcc_output


	.area _CODE

	.globl _main
_main:
	; prologue: main (locals=164)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-164
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L0:
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #39
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_89383
	ld	hl, #0
	jp	__cmp_e_30886
__cmp_t_89383:
	ld	hl, #1
__cmp_e_30886:
	dec	sp
	dec	sp
	ld	-166(ix), l
	ld	-165(ix), h
	ld	l, -166(ix)
	ld	h, -165(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L3
__xcc_L1:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-168(ix), l
	ld	-167(ix), h
	ld	l, -86(ix)
	ld	h, -85(ix)
	ld	e, -168(ix)
	ld	d, -167(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-170(ix), l
	ld	-169(ix), h
	ld	l, -170(ix)
	ld	h, -169(ix)
	push	hl
	ld	e, -4(ix)
	ld	d, -3(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-172(ix), l
	ld	-171(ix), h
	ld	l, -164(ix)
	ld	h, -163(ix)
	ld	e, -172(ix)
	ld	d, -171(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-174(ix), l
	ld	-173(ix), h
	ld	l, -174(ix)
	ld	h, -173(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L2:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	ld	-176(ix), l
	ld	-175(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	inc	hl
	ld	-4(ix), l
	ld	-3(ix), h
	jp	__xcc_L0
__xcc_L3:
	ld	l, -86(ix)
	ld	h, -85(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -164(ix)
	ld	h, -163(ix)
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #39
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	de, #7
	add	hl, de
	dec	sp
	dec	sp
	ld	-178(ix), l
	ld	-177(ix), h
	ld	hl, #8
	push	hl
	ld	l, -178(ix)
	ld	h, -177(ix)
	pop	de
	.globl __divsint
	call	__divsint
	ex	de, hl
	dec	sp
	dec	sp
	ld	-180(ix), l
	ld	-179(ix), h
	ld	l, -180(ix)
	ld	h, -179(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #8
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	pop	de
	.globl __smod16
	call	__smod16
	dec	sp
	dec	sp
	ld	-182(ix), l
	ld	-181(ix), h
	ld	l, -182(ix)
	ld	h, -181(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	z, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-184(ix), l
	ld	-183(ix), h
	ld	l, -184(ix)
	ld	h, -183(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L4
	jp	__xcc_L5
__xcc_L4:
__xcc_L6:
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-186(ix), l
	ld	-185(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -186(ix)
	ld	h, -185(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-188(ix), l
	ld	-187(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	dec	sp
	dec	sp
	ld	-190(ix), l
	ld	-189(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	inc	hl
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -190(ix)
	ld	h, -189(ix)
	push	hl
	ld	e, -188(ix)
	ld	d, -187(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-192(ix), l
	ld	-191(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -192(ix)
	ld	h, -191(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-194(ix), l
	ld	-193(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	dec	sp
	dec	sp
	ld	-196(ix), l
	ld	-195(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	inc	hl
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -196(ix)
	ld	h, -195(ix)
	push	hl
	ld	e, -194(ix)
	ld	d, -193(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-198(ix), l
	ld	-197(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -198(ix)
	ld	h, -197(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-200(ix), l
	ld	-199(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	dec	sp
	dec	sp
	ld	-202(ix), l
	ld	-201(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	inc	hl
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -202(ix)
	ld	h, -201(ix)
	push	hl
	ld	e, -200(ix)
	ld	d, -199(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-204(ix), l
	ld	-203(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -204(ix)
	ld	h, -203(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-206(ix), l
	ld	-205(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	dec	sp
	dec	sp
	ld	-208(ix), l
	ld	-207(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	inc	hl
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -208(ix)
	ld	h, -207(ix)
	push	hl
	ld	e, -206(ix)
	ld	d, -205(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-210(ix), l
	ld	-209(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
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
	ld	l, -8(ix)
	ld	h, -7(ix)
	dec	sp
	dec	sp
	ld	-214(ix), l
	ld	-213(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	inc	hl
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -214(ix)
	ld	h, -213(ix)
	push	hl
	ld	e, -212(ix)
	ld	d, -211(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-216(ix), l
	ld	-215(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -216(ix)
	ld	h, -215(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-218(ix), l
	ld	-217(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	dec	sp
	dec	sp
	ld	-220(ix), l
	ld	-219(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	inc	hl
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -220(ix)
	ld	h, -219(ix)
	push	hl
	ld	e, -218(ix)
	ld	d, -217(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-222(ix), l
	ld	-221(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -222(ix)
	ld	h, -221(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-224(ix), l
	ld	-223(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	dec	sp
	dec	sp
	ld	-226(ix), l
	ld	-225(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	inc	hl
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -226(ix)
	ld	h, -225(ix)
	push	hl
	ld	e, -224(ix)
	ld	d, -223(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -6(ix)
	ld	h, -5(ix)
	dec	sp
	dec	sp
	ld	-228(ix), l
	ld	-227(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -228(ix)
	ld	h, -227(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-230(ix), l
	ld	-229(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	dec	sp
	dec	sp
	ld	-232(ix), l
	ld	-231(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	inc	hl
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -232(ix)
	ld	h, -231(ix)
	push	hl
	ld	e, -230(ix)
	ld	d, -229(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__xcc_L7:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	hl
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	z, __cmp_e_38335
	jp	p, __cmp_t_47793
	ld	hl, #0
	jp	__cmp_e_38335
__cmp_t_47793:
	ld	hl, #1
__cmp_e_38335:
	dec	sp
	dec	sp
	ld	-234(ix), l
	ld	-233(ix), h
	ld	l, -234(ix)
	ld	h, -233(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L8
__xcc_L8:
__xcc_L5:
	ld	hl, #0
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L9:
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #39
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_85386
	ld	hl, #0
	jp	__cmp_e_60492
__cmp_t_85386:
	ld	hl, #1
__cmp_e_60492:
	dec	sp
	dec	sp
	ld	-236(ix), l
	ld	-235(ix), h
	ld	l, -236(ix)
	ld	h, -235(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L10
	jp	__xcc_L12
__xcc_L10:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-238(ix), l
	ld	-237(ix), h
	ld	l, -86(ix)
	ld	h, -85(ix)
	ld	e, -238(ix)
	ld	d, -237(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-240(ix), l
	ld	-239(ix), h
	ld	l, -240(ix)
	ld	h, -239(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-242(ix), l
	ld	-241(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-244(ix), l
	ld	-243(ix), h
	ld	l, -164(ix)
	ld	h, -163(ix)
	ld	e, -244(ix)
	ld	d, -243(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-246(ix), l
	ld	-245(ix), h
	ld	l, -246(ix)
	ld	h, -245(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-248(ix), l
	ld	-247(ix), h
	ld	l, -242(ix)
	ld	h, -241(ix)
	push	hl
	ld	l, -248(ix)
	ld	h, -247(ix)
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-250(ix), l
	ld	-249(ix), h
	ld	l, -250(ix)
	ld	h, -249(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L13
	jp	__xcc_L15
__xcc_L13:
	ld	hl, #1
	jp	__main_end
	jp	__xcc_L15
__xcc_L15:
__xcc_L11:
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	ld	-252(ix), l
	ld	-251(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	inc	hl
	ld	-4(ix), l
	ld	-3(ix), h
	jp	__xcc_L9
__xcc_L12:
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
