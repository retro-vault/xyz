	.module	xcc_output
	.area	_DATA
_main__raw_0:
	.ds	96
_main__stream_1:
	.ds	96
	.area	_CODE
	.globl	_main
_main:
	; sdcccall(1) prologue: main (locals=0, temp_frame=23, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-23
	add	hl, sp
	ld	sp, hl
__xcc_L3:
	ld	hl, (#65296)
	ld	b, h
	ld	c, l
	ld	a, l
	xor	#120
	ld	l, a
	ld	a, h
	xor	#0
	ld	h, a
	ld	-21(ix), l
	ld	-20(ix), h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	b, h
	ld	c, l
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	-23(ix), l
	ld	-22(ix), h
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	ld	b, h
	ld	c, l
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	e, #169
	ld	d, #0
	add	hl, de
	ld	a, l
	ld	-10(ix), a
	xor	a
	ld	-14(ix), a
__xcc_L6:
	ld	a, -14(ix)
	cp	#96
	jr	nc, __xcc_L5
__xcc_L7:
	ld	a, -10(ix)
	add	a, a
	add	a, a
	add	a, a
	ld	-20(ix), a
	ld	a, -10(ix)
	ld	e, a
	ld	a, -20(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-21(ix), a
	srl	a
	srl	a
	srl	a
	srl	a
	srl	a
	ld	-20(ix), a
	ld	a, -21(ix)
	ld	e, a
	ld	a, -20(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-22(ix), a
	ld	a, -14(ix)
	add	a, #34
	add	a, #17
	ld	-20(ix), a
	ld	a, -22(ix)
	ld	e, a
	ld	a, -20(ix)
	ld	d, a
	ld	a, e
	add	a, d
	ld	-10(ix), a
	ld	a, -14(ix)
	ld	l, a
	ld	h, #0
	ld	-21(ix), l
	ld	-20(ix), h
	ld	a, -10(ix)
	ld	e, a
	ld	a, -14(ix)
	ld	d, a
	ld	a, e
	xor	a, d
	ld	-22(ix), a
	ld	hl, #_main__raw_0
	ld	e, -21(ix)
	ld	d, -20(ix)
	add	hl, de
	ld	(hl), a
__xcc_L8:
	ld	a, -14(ix)
	add	a, #1
	ld	-14(ix), a
	jp	__xcc_L6
__xcc_L5:
	xor	a
	ld	-15(ix), a
__xcc_L10:
	ld	a, -15(ix)
	cp	#96
	jp	nc, __xcc_L13
__xcc_L11:
	ld	a, -15(ix)
	ld	e, a
	ld	d, #0
	ld	hl, #_main__raw_0
	add	hl, de
	ld	a, (hl)
	ld	-18(ix), a
	and	#7
	ld	-13(ix), a
	; O3 jump-table switch (7 cases, span=7)
	cp	#7
	jp	nc, __xcc_L21
	add	a, a
	ld	e, a
	ld	d, #0
	ld	hl, #__main_swtab_40
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	jp	(hl)
__main_swtab_40:
	.dw	__xcc_L14
	.dw	__xcc_L15
	.dw	__xcc_L16
	.dw	__xcc_L17
	.dw	__xcc_L18
	.dw	__xcc_L19
	.dw	__xcc_L20
__xcc_L14:
	ld	a, -18(ix)
	and	#9
	ld	-20(ix), a
	add	a, #48
	ld	-21(ix), a
	ld	hl, #_main__stream_1
	ld	e, -15(ix)
	ld	d, #0
	add	hl, de
	ld	(hl), a
	jp	__xcc_L12
__xcc_L15:
	ld	a, -18(ix)
	and	#15
	ld	-20(ix), a
	add	a, #97
	ld	-21(ix), a
	ld	hl, #_main__stream_1
	ld	e, -15(ix)
	ld	d, #0
	add	hl, de
	ld	(hl), a
	jp	__xcc_L12
__xcc_L16:
	ld	a, -18(ix)
	and	#7
	ld	-20(ix), a
	add	a, #65
	ld	-21(ix), a
	ld	hl, #_main__stream_1
	ld	e, -15(ix)
	ld	d, #0
	add	hl, de
	ld	(hl), a
	jp	__xcc_L12
__xcc_L17:
	ld	a, -18(ix)
	and	#1
	ld	-20(ix), a
	or	a, a
	jr	z, __xcc_L24
__xcc_L23:
	ld	hl, #43
	ld	-9(ix), l
	ld	-8(ix), h
	jr	__xcc_L25
__xcc_L24:
	ld	hl, #45
	ld	-9(ix), l
	ld	-8(ix), h
__xcc_L25:
	ld	hl, #_main__stream_1
	ld	e, -15(ix)
	ld	d, #0
	add	hl, de
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -9(ix)
	ld	h, -8(ix)
	ld	a, l
	ld	-22(ix), a
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	(hl), a
	jr	__xcc_L12
__xcc_L18:
	ld	hl, #_main__stream_1
	ld	e, -15(ix)
	ld	d, #0
	add	hl, de
	ld	a, #46
	ld	(hl), a
	jr	__xcc_L12
__xcc_L19:
	ld	a, -18(ix)
	and	#1
	ld	-20(ix), a
	or	a, a
	jr	z, __xcc_L27
__xcc_L26:
	ld	hl, #101
	ld	-17(ix), l
	ld	-16(ix), h
	jr	__xcc_L28
__xcc_L27:
	ld	hl, #69
	ld	-17(ix), l
	ld	-16(ix), h
__xcc_L28:
	ld	hl, #_main__stream_1
	ld	e, -15(ix)
	ld	d, #0
	add	hl, de
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -17(ix)
	ld	h, -16(ix)
	ld	a, l
	ld	-22(ix), a
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	(hl), a
	jr	__xcc_L12
__xcc_L20:
	ld	hl, #_main__stream_1
	ld	e, -15(ix)
	ld	d, #0
	add	hl, de
	ld	a, #44
	ld	(hl), a
	jr	__xcc_L12
__xcc_L21:
	ld	hl, #_main__stream_1
	ld	e, -15(ix)
	ld	d, #0
	add	hl, de
	ld	a, #32
	ld	(hl), a
__xcc_L22:
__xcc_L12:
	ld	a, -15(ix)
	add	a, #1
	ld	-15(ix), a
	jp	__xcc_L10
__xcc_L13:
	xor	a
	ld	-19(ix), a
	ld	hl, #0
	ld	-7(ix), l
	ld	-6(ix), h
	ld	hl, #0
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #0
	ld	-5(ix), l
	ld	-4(ix), h
	ld	hl, #9320
	ld	-3(ix), l
	ld	-2(ix), h
	xor	a
	ld	-15(ix), a
__xcc_L29:
	ld	a, -15(ix)
	cp	#96
	jp	nc, __xcc_L32
__xcc_L30:
	ld	a, -15(ix)
	ld	e, a
	ld	d, #0
	ld	hl, #_main__stream_1
	add	hl, de
	ld	a, (hl)
	ld	-1(ix), a
	cp	#32
	jr	z, __xcc_L33
__xcc_L36:
	ld	a, -1(ix)
	cp	#44
	jr	z, __xcc_L33
	jr	__xcc_L35
__xcc_L33:
	ld	a, -19(ix)
	cp	#2
	jr	z, __xcc_L37
__xcc_L41:
	ld	a, -19(ix)
	cp	#4
	jr	z, __xcc_L37
__xcc_L40:
	ld	a, -19(ix)
	cp	#6
	jr	z, __xcc_L37
	jr	__xcc_L39
__xcc_L37:
	ld	l, -7(ix)
	ld	h, -6(ix)
	inc	hl
	ld	-7(ix), l
	ld	-6(ix), h
__xcc_L39:
	xor	a
	ld	-19(ix), a
	ld	l, -5(ix)
	ld	h, -4(ix)
	inc	hl
	ld	-5(ix), l
	ld	-4(ix), h
	jp	__xcc_L31
__xcc_L35:
	; O3 jump-table switch (7 cases, span=7)
	ld	a, -19(ix)
	cp	#7
	jp	nc, __xcc_L49
	add	a, a
	ld	e, a
	ld	d, #0
	ld	hl, #__main_swtab_150
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	jp	(hl)
__main_swtab_150:
	.dw	__xcc_L42
	.dw	__xcc_L43
	.dw	__xcc_L44
	.dw	__xcc_L47
	.dw	__xcc_L45
	.dw	__xcc_L46
	.dw	__xcc_L48
__xcc_L42:
	ld	a, -1(ix)
	cp	#43
	jr	z, __xcc_L51
__xcc_L54:
	ld	a, -1(ix)
	cp	#45
	jr	z, __xcc_L51
	jr	__xcc_L52
__xcc_L51:
	ld	a, #1
	ld	-19(ix), a
	jr	__xcc_L53
__xcc_L52:
	ld	a, -1(ix)
	xor	#128
	cp	#176
	jr	c, __xcc_L56
__xcc_L58:
	ld	a, -1(ix)
	xor	#128
	cp	#185
	jr	z, __xcc_L55
	jr	c, __xcc_L55
	jr	__xcc_L56
__xcc_L55:
	ld	a, #2
	ld	-19(ix), a
	jr	__xcc_L53
__xcc_L56:
	ld	a, #7
	ld	-19(ix), a
	ld	l, -12(ix)
	ld	h, -11(ix)
	inc	hl
	ld	-12(ix), l
	ld	-11(ix), h
__xcc_L57:
__xcc_L53:
	ld	l, -5(ix)
	ld	h, -4(ix)
	inc	hl
	ld	-5(ix), l
	ld	-4(ix), h
	jp	__xcc_L50
__xcc_L43:
	ld	a, -1(ix)
	xor	#128
	cp	#176
	jr	c, __xcc_L60
__xcc_L62:
	ld	a, -1(ix)
	xor	#128
	cp	#185
	jr	z, __xcc_L59
	jr	c, __xcc_L59
	jr	__xcc_L60
__xcc_L59:
	ld	a, #2
	ld	-19(ix), a
	jr	__xcc_L61
__xcc_L60:
	ld	a, #7
	ld	-19(ix), a
	ld	l, -12(ix)
	ld	h, -11(ix)
	inc	hl
	ld	-12(ix), l
	ld	-11(ix), h
__xcc_L61:
	ld	l, -5(ix)
	ld	h, -4(ix)
	inc	hl
	ld	-5(ix), l
	ld	-4(ix), h
	jp	__xcc_L50
__xcc_L44:
	ld	a, -1(ix)
	xor	#128
	cp	#176
	jr	c, __xcc_L64
__xcc_L66:
	ld	a, -1(ix)
	xor	#128
	cp	#185
	jp	z, __xcc_L50
	jp	c, __xcc_L50
__xcc_L64:
	ld	a, -1(ix)
	cp	#46
	jr	nz, __xcc_L68
__xcc_L67:
	ld	a, #4
	ld	-19(ix), a
	jp	__xcc_L50
__xcc_L68:
	ld	a, -1(ix)
	cp	#101
	jr	z, __xcc_L70
__xcc_L73:
	ld	a, -1(ix)
	cp	#69
	jr	z, __xcc_L70
	jr	__xcc_L71
__xcc_L70:
	ld	a, #5
	ld	-19(ix), a
	jp	__xcc_L50
__xcc_L71:
	ld	a, #7
	ld	-19(ix), a
	ld	l, -12(ix)
	ld	h, -11(ix)
	inc	hl
	ld	-12(ix), l
	ld	-11(ix), h
__xcc_L72:
__xcc_L69:
__xcc_L65:
	jp	__xcc_L50
__xcc_L45:
	ld	a, -1(ix)
	xor	#128
	cp	#176
	jr	c, __xcc_L75
__xcc_L77:
	ld	a, -1(ix)
	xor	#128
	cp	#185
	jp	z, __xcc_L50
	jp	c, __xcc_L50
__xcc_L75:
	ld	a, -1(ix)
	cp	#101
	jr	z, __xcc_L78
__xcc_L81:
	ld	a, -1(ix)
	cp	#69
	jr	z, __xcc_L78
	jr	__xcc_L79
__xcc_L78:
	ld	a, #5
	ld	-19(ix), a
	jp	__xcc_L50
__xcc_L79:
	ld	a, #7
	ld	-19(ix), a
	ld	l, -12(ix)
	ld	h, -11(ix)
	inc	hl
	ld	-12(ix), l
	ld	-11(ix), h
__xcc_L80:
__xcc_L76:
	jp	__xcc_L50
__xcc_L46:
	ld	a, -1(ix)
	cp	#43
	jr	z, __xcc_L82
__xcc_L85:
	ld	a, -1(ix)
	cp	#45
	jr	z, __xcc_L82
	jr	__xcc_L83
__xcc_L82:
	ld	a, #3
	ld	-19(ix), a
	jr	__xcc_L84
__xcc_L83:
	ld	a, -1(ix)
	xor	#128
	cp	#176
	jr	c, __xcc_L87
__xcc_L89:
	ld	a, -1(ix)
	xor	#128
	cp	#185
	jr	z, __xcc_L86
	jr	c, __xcc_L86
	jr	__xcc_L87
__xcc_L86:
	ld	a, #6
	ld	-19(ix), a
	jr	__xcc_L84
__xcc_L87:
	ld	a, #7
	ld	-19(ix), a
	ld	l, -12(ix)
	ld	h, -11(ix)
	inc	hl
	ld	-12(ix), l
	ld	-11(ix), h
__xcc_L88:
__xcc_L84:
	ld	l, -5(ix)
	ld	h, -4(ix)
	inc	hl
	ld	-5(ix), l
	ld	-4(ix), h
	jp	__xcc_L50
__xcc_L47:
	ld	a, -1(ix)
	xor	#128
	cp	#176
	jr	c, __xcc_L91
__xcc_L93:
	ld	a, -1(ix)
	xor	#128
	cp	#185
	jr	z, __xcc_L90
	jr	c, __xcc_L90
	jr	__xcc_L91
__xcc_L90:
	ld	a, #6
	ld	-19(ix), a
	jr	__xcc_L92
__xcc_L91:
	ld	a, #7
	ld	-19(ix), a
	ld	l, -12(ix)
	ld	h, -11(ix)
	inc	hl
	ld	-12(ix), l
	ld	-11(ix), h
__xcc_L92:
	ld	l, -5(ix)
	ld	h, -4(ix)
	inc	hl
	ld	-5(ix), l
	ld	-4(ix), h
	jr	__xcc_L50
__xcc_L48:
	ld	a, -1(ix)
	xor	#128
	cp	#176
	jr	c, __xcc_L95
__xcc_L97:
	ld	a, -1(ix)
	xor	#128
	cp	#185
	jr	z, __xcc_L50
	jr	c, __xcc_L50
__xcc_L95:
	ld	a, #7
	ld	-19(ix), a
	ld	l, -12(ix)
	ld	h, -11(ix)
	inc	hl
	ld	-12(ix), l
	ld	-11(ix), h
__xcc_L96:
	jr	__xcc_L50
__xcc_L49:
	ld	a, -1(ix)
	xor	#128
	cp	#225
	jr	c, __xcc_L101
__xcc_L102:
	ld	a, -1(ix)
	xor	#128
	cp	#250
	jr	z, __xcc_L50
	jr	c, __xcc_L50
__xcc_L101:
	ld	a, -1(ix)
	xor	#128
	cp	#193
	jr	c, __xcc_L99
__xcc_L103:
	ld	a, -1(ix)
	xor	#128
	cp	#218
	jr	z, __xcc_L50
	jr	c, __xcc_L50
__xcc_L99:
	xor	a
	ld	-19(ix), a
__xcc_L100:
__xcc_L50:
	ld	a, -1(ix)
	ld	l, a
	ld	h, #0
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -3(ix)
	ld	h, -2(ix)
	ld	-23(ix), l
	ld	-22(ix), h
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	b, h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	a, b
	rrca
	rrca
	rrca
	and	#31
	or	l
	ld	l, a
	ld	-23(ix), l
	ld	-22(ix), h
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-21(ix), l
	ld	-20(ix), h
	ld	a, -19(ix)
	ld	l, a
	ld	h, #0
	ld	-23(ix), l
	ld	-22(ix), h
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	b, h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	a, b
	rrca
	rrca
	rrca
	and	#31
	or	l
	ld	l, a
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-3(ix), l
	ld	-2(ix), h
__xcc_L31:
	ld	a, -15(ix)
	add	a, #1
	ld	-15(ix), a
	jp	__xcc_L29
__xcc_L32:
	ld	l, -7(ix)
	ld	h, -6(ix)
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -3(ix)
	ld	h, -2(ix)
	ld	-23(ix), l
	ld	-22(ix), h
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	b, h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	a, b
	rrca
	rrca
	rrca
	and	#31
	or	l
	ld	l, a
	ld	-23(ix), l
	ld	-22(ix), h
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	-23(ix), l
	ld	-22(ix), h
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	b, h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	a, b
	rrca
	rrca
	rrca
	and	#31
	or	l
	ld	l, a
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-23(ix), l
	ld	-22(ix), h
	ld	l, -5(ix)
	ld	h, -4(ix)
	ld	-21(ix), l
	ld	-20(ix), h
	ld	de, #40503
	add	hl, de
	ld	b, h
	ld	c, l
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	d, b
	ld	e, c
	ld	a, l
	xor	a, e
	ld	l, a
	ld	a, h
	xor	a, d
	ld	h, a
	ld	b, h
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	a, b
	rrca
	rrca
	rrca
	and	#31
	or	l
	ld	l, a
	ld	-23(ix), l
	ld	-22(ix), h
	ld	l, -21(ix)
	ld	h, -20(ix)
	ld	a, l
	xor	#74
	ld	l, a
	ld	a, h
	xor	#127
	ld	h, a
	ld	b, h
	ld	c, l
	ld	l, -23(ix)
	ld	h, -22(ix)
	ld	d, b
	ld	e, c
	add	hl, de
	ld	-21(ix), l
	ld	-20(ix), h
	ex	de, hl
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
