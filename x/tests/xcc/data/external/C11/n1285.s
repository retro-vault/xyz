	.module xcc_output


	.area _CODE

	.globl _func_return
_func_return:
	; prologue: func_return (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	.globl _f
	call	_f
	ld	-12(ix), l
	ld	-11(ix), h
	push	ix
	pop	hl
	ld	de, #-12
	add	hl, de
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	.globl _f
	call	_f
	ld	-26(ix), l
	ld	-25(ix), h
	push	ix
	pop	hl
	ld	de, #-26
	add	hl, de
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	.globl _g
	call	_g
	ld	-50(ix), l
	ld	-49(ix), h
	push	ix
	pop	hl
	ld	de, #-50
	add	hl, de
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	.globl _f
	call	_f
	ld	-64(ix), l
	ld	-63(ix), h
	push	ix
	pop	hl
	ld	de, #-64
	add	hl, de
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -66(ix)
	ld	h, -65(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	push	hl
	.globl _sink
	call	_sink
	pop	bc
	.globl _f
	call	_f
	ld	-78(ix), l
	ld	-77(ix), h
	push	ix
	pop	hl
	ld	de, #-78
	add	hl, de
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	ld	l, -80(ix)
	ld	h, -79(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	l, -82(ix)
	ld	h, -81(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-84(ix), l
	ld	-83(ix), h
	ld	l, -84(ix)
	ld	h, -83(ix)
	jp	__func_return_end
__func_return_end:
	; epilogue: func_return
	ld	sp, ix
	pop	ix
	ret
	.globl _ternary
_ternary:
	; prologue: ternary (locals=106)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-106
	add	hl, sp
	ld	sp, hl
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L0
	jp	__xcc_L1
__xcc_L0:
	push	ix
	pop	hl
	ld	de, #-12
	add	hl, de
	dec	sp
	dec	sp
	ld	-108(ix), l
	ld	-107(ix), h
	ld	l, -108(ix)
	ld	h, -107(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -12(ix)
	ld	h, -11(ix)
	dec	sp
	dec	sp
	ld	-110(ix), l
	ld	-109(ix), h
	jp	__xcc_L2
__xcc_L1:
	.globl _f
	call	_f
	ld	-120(ix), l
	ld	-119(ix), h
	ld	l, -120(ix)
	ld	h, -119(ix)
	ld	-110(ix), l
	ld	-109(ix), h
__xcc_L2:
	push	ix
	pop	hl
	ld	de, #-110
	add	hl, de
	dec	sp
	dec	sp
	ld	-122(ix), l
	ld	-121(ix), h
	ld	l, -122(ix)
	ld	h, -121(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-124(ix), l
	ld	-123(ix), h
	ld	l, -124(ix)
	ld	h, -123(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L3
	jp	__xcc_L4
__xcc_L3:
	push	ix
	pop	hl
	ld	de, #-34
	add	hl, de
	dec	sp
	dec	sp
	ld	-126(ix), l
	ld	-125(ix), h
	ld	l, -126(ix)
	ld	h, -125(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -34(ix)
	ld	h, -33(ix)
	dec	sp
	dec	sp
	ld	-128(ix), l
	ld	-127(ix), h
	jp	__xcc_L5
__xcc_L4:
	.globl _g
	call	_g
	ld	-148(ix), l
	ld	-147(ix), h
	ld	l, -148(ix)
	ld	h, -147(ix)
	ld	-128(ix), l
	ld	-127(ix), h
__xcc_L5:
	push	ix
	pop	hl
	ld	de, #-128
	add	hl, de
	dec	sp
	dec	sp
	ld	-150(ix), l
	ld	-149(ix), h
	ld	l, -150(ix)
	ld	h, -149(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-152(ix), l
	ld	-151(ix), h
	ld	l, -152(ix)
	ld	h, -151(ix)
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L6
	jp	__xcc_L7
__xcc_L6:
	push	ix
	pop	hl
	ld	de, #-44
	add	hl, de
	dec	sp
	dec	sp
	ld	-154(ix), l
	ld	-153(ix), h
	ld	l, -154(ix)
	ld	h, -153(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -44(ix)
	ld	h, -43(ix)
	dec	sp
	dec	sp
	ld	-156(ix), l
	ld	-155(ix), h
	jp	__xcc_L8
__xcc_L7:
	.globl _f
	call	_f
	ld	-166(ix), l
	ld	-165(ix), h
	ld	l, -166(ix)
	ld	h, -165(ix)
	ld	-156(ix), l
	ld	-155(ix), h
__xcc_L8:
	push	ix
	pop	hl
	ld	de, #-156
	add	hl, de
	dec	sp
	dec	sp
	ld	-168(ix), l
	ld	-167(ix), h
	ld	l, -168(ix)
	ld	h, -167(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-170(ix), l
	ld	-169(ix), h
	ld	l, -170(ix)
	ld	h, -169(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L9
	jp	__xcc_L10
__xcc_L9:
	push	ix
	pop	hl
	ld	de, #-54
	add	hl, de
	dec	sp
	dec	sp
	ld	-172(ix), l
	ld	-171(ix), h
	ld	l, -172(ix)
	ld	h, -171(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -54(ix)
	ld	h, -53(ix)
	dec	sp
	dec	sp
	ld	-174(ix), l
	ld	-173(ix), h
	jp	__xcc_L11
__xcc_L10:
	.globl _f
	call	_f
	ld	-184(ix), l
	ld	-183(ix), h
	ld	l, -184(ix)
	ld	h, -183(ix)
	ld	-174(ix), l
	ld	-173(ix), h
__xcc_L11:
	push	ix
	pop	hl
	ld	de, #-174
	add	hl, de
	dec	sp
	dec	sp
	ld	-186(ix), l
	ld	-185(ix), h
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
	ld	l, -188(ix)
	ld	h, -187(ix)
	push	hl
	.globl _sink
	call	_sink
	pop	bc
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L12
	jp	__xcc_L13
__xcc_L12:
	push	ix
	pop	hl
	ld	de, #-66
	add	hl, de
	dec	sp
	dec	sp
	ld	-190(ix), l
	ld	-189(ix), h
	ld	l, -190(ix)
	ld	h, -189(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-66
	add	hl, de
	dec	sp
	dec	sp
	ld	-192(ix), l
	ld	-191(ix), h
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
	ld	l, -194(ix)
	ld	h, -193(ix)
	dec	sp
	dec	sp
	ld	-196(ix), l
	ld	-195(ix), h
	jp	__xcc_L14
__xcc_L13:
	.globl _f
	call	_f
	ld	-206(ix), l
	ld	-205(ix), h
	push	ix
	pop	hl
	ld	de, #-206
	add	hl, de
	dec	sp
	dec	sp
	ld	-208(ix), l
	ld	-207(ix), h
	ld	l, -208(ix)
	ld	h, -207(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-210(ix), l
	ld	-209(ix), h
	ld	l, -210(ix)
	ld	h, -209(ix)
	ld	-196(ix), l
	ld	-195(ix), h
__xcc_L14:
	ld	l, -196(ix)
	ld	h, -195(ix)
	ld	-56(ix), l
	ld	-55(ix), h
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L15
	jp	__xcc_L16
__xcc_L15:
	push	ix
	pop	hl
	ld	de, #-76
	add	hl, de
	dec	sp
	dec	sp
	ld	-212(ix), l
	ld	-211(ix), h
	ld	l, -212(ix)
	ld	h, -211(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-76
	add	hl, de
	dec	sp
	dec	sp
	ld	-214(ix), l
	ld	-213(ix), h
	ld	l, -214(ix)
	ld	h, -213(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-216(ix), l
	ld	-215(ix), h
	ld	l, -216(ix)
	ld	h, -215(ix)
	dec	sp
	dec	sp
	ld	-218(ix), l
	ld	-217(ix), h
	jp	__xcc_L17
__xcc_L16:
	.globl _f
	call	_f
	ld	-228(ix), l
	ld	-227(ix), h
	push	ix
	pop	hl
	ld	de, #-228
	add	hl, de
	dec	sp
	dec	sp
	ld	-230(ix), l
	ld	-229(ix), h
	ld	l, -230(ix)
	ld	h, -229(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-232(ix), l
	ld	-231(ix), h
	ld	l, -232(ix)
	ld	h, -231(ix)
	ld	-218(ix), l
	ld	-217(ix), h
__xcc_L17:
	ld	l, -218(ix)
	ld	h, -217(ix)
	ld	-56(ix), l
	ld	-55(ix), h
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L18
	jp	__xcc_L19
__xcc_L18:
	push	ix
	pop	hl
	ld	de, #-86
	add	hl, de
	dec	sp
	dec	sp
	ld	-234(ix), l
	ld	-233(ix), h
	ld	l, -234(ix)
	ld	h, -233(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-86
	add	hl, de
	dec	sp
	dec	sp
	ld	-236(ix), l
	ld	-235(ix), h
	ld	l, -236(ix)
	ld	h, -235(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-238(ix), l
	ld	-237(ix), h
	ld	l, -238(ix)
	ld	h, -237(ix)
	dec	sp
	dec	sp
	ld	-240(ix), l
	ld	-239(ix), h
	jp	__xcc_L20
__xcc_L19:
	.globl _g
	call	_g
	ld	-260(ix), l
	ld	-259(ix), h
	push	ix
	pop	hl
	ld	de, #-260
	add	hl, de
	dec	sp
	dec	sp
	ld	-262(ix), l
	ld	-261(ix), h
	ld	l, -262(ix)
	ld	h, -261(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-264(ix), l
	ld	-263(ix), h
	ld	l, -264(ix)
	ld	h, -263(ix)
	ld	-240(ix), l
	ld	-239(ix), h
__xcc_L20:
	ld	l, -240(ix)
	ld	h, -239(ix)
	ld	-56(ix), l
	ld	-55(ix), h
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L21
	jp	__xcc_L22
__xcc_L21:
	push	ix
	pop	hl
	ld	de, #-96
	add	hl, de
	dec	sp
	dec	sp
	ld	-266(ix), l
	ld	-265(ix), h
	ld	l, -266(ix)
	ld	h, -265(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-96
	add	hl, de
	dec	sp
	dec	sp
	ld	-268(ix), l
	ld	-267(ix), h
	ld	l, -268(ix)
	ld	h, -267(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-270(ix), l
	ld	-269(ix), h
	ld	l, -270(ix)
	ld	h, -269(ix)
	dec	sp
	dec	sp
	ld	-272(ix), l
	ld	-271(ix), h
	jp	__xcc_L23
__xcc_L22:
	.globl _f
	call	_f
	ld	-282(ix), l
	ld	-281(ix), h
	push	ix
	pop	hl
	ld	de, #-282
	add	hl, de
	dec	sp
	dec	sp
	ld	-284(ix), l
	ld	-283(ix), h
	ld	l, -284(ix)
	ld	h, -283(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-286(ix), l
	ld	-285(ix), h
	ld	l, -286(ix)
	ld	h, -285(ix)
	ld	-272(ix), l
	ld	-271(ix), h
__xcc_L23:
	ld	l, -272(ix)
	ld	h, -271(ix)
	push	hl
	.globl _sink
	call	_sink
	pop	bc
	ld	hl, #1
	ld	a, h
	or	a, l
	jp	nz, __xcc_L24
	jp	__xcc_L25
__xcc_L24:
	push	ix
	pop	hl
	ld	de, #-106
	add	hl, de
	dec	sp
	dec	sp
	ld	-288(ix), l
	ld	-287(ix), h
	ld	l, -288(ix)
	ld	h, -287(ix)
	push	hl
	ld	de, #0
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-106
	add	hl, de
	dec	sp
	dec	sp
	ld	-290(ix), l
	ld	-289(ix), h
	ld	l, -290(ix)
	ld	h, -289(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-292(ix), l
	ld	-291(ix), h
	ld	l, -292(ix)
	ld	h, -291(ix)
	dec	sp
	dec	sp
	ld	-294(ix), l
	ld	-293(ix), h
	jp	__xcc_L26
__xcc_L25:
	.globl _f
	call	_f
	ld	-304(ix), l
	ld	-303(ix), h
	push	ix
	pop	hl
	ld	de, #-304
	add	hl, de
	dec	sp
	dec	sp
	ld	-306(ix), l
	ld	-305(ix), h
	ld	l, -306(ix)
	ld	h, -305(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-308(ix), l
	ld	-307(ix), h
	ld	l, -308(ix)
	ld	h, -307(ix)
	ld	-294(ix), l
	ld	-293(ix), h
__xcc_L26:
	ld	l, -294(ix)
	ld	h, -293(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-310(ix), l
	ld	-309(ix), h
	ld	l, -310(ix)
	ld	h, -309(ix)
	jp	__ternary_end
__ternary_end:
	; epilogue: ternary
	ld	sp, ix
	pop	ix
	ret
	.globl _comma
_comma:
	; prologue: comma (locals=12)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-12
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	.globl _sink
	call	_sink
	pop	bc
	ld	hl, #0
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	jp	__comma_end
__comma_end:
	; epilogue: comma
	ld	sp, ix
	pop	ix
	ret
	.globl _cast
_cast:
	; prologue: cast (locals=12)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-12
	add	hl, sp
	ld	sp, hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-22(ix), l
	ld	-21(ix), h
	push	ix
	pop	hl
	ld	de, #-22
	add	hl, de
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-36(ix), l
	ld	-35(ix), h
	push	ix
	pop	hl
	ld	de, #-36
	add	hl, de
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-50(ix), l
	ld	-49(ix), h
	push	ix
	pop	hl
	ld	de, #-50
	add	hl, de
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	.globl _sink
	call	_sink
	pop	bc
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	-64(ix), l
	ld	-63(ix), h
	push	ix
	pop	hl
	ld	de, #-64
	add	hl, de
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -66(ix)
	ld	h, -65(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	l, -70(ix)
	ld	h, -69(ix)
	jp	__cast_end
__cast_end:
	; epilogue: cast
	ld	sp, ix
	pop	ix
	ret
	.globl _assign
_assign:
	; prologue: assign (locals=22)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-22
	add	hl, sp
	ld	sp, hl
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	-10(ix), l
	ld	-9(ix), h
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	-10(ix), l
	ld	-9(ix), h
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	-10(ix), l
	ld	-9(ix), h
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	push	hl
	.globl _sink
	call	_sink
	pop	bc
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	-10(ix), l
	ld	-9(ix), h
	push	ix
	pop	hl
	ld	de, #-10
	add	hl, de
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	l, -36(ix)
	ld	h, -35(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	jp	__assign_end
__assign_end:
	; epilogue: assign
	ld	sp, ix
	pop	ix
	ret
