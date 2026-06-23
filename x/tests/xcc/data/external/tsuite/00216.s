	.module xcc_output

	.area _DATA
	.globl _ce
_ce:
	.dw 1
	.ds 0
	.db 18
	.globl _sinit16
_sinit16:
	.ds 3
	.dw 2
	.dw 0
	.globl _gs
_gs:
	.ds 4
	.globl _gs2
_gs2:
	.db 1
	.db 2
	.db 3
	.db 4
	.globl _gt
_gt:
	.ds 16
	.db 42
	.globl _gu
_gu:
	.db 3
	.dw 5
	.dw 0
	.db 6
	.ds 17
	.dw 8
	.dw 4
	.dw 0
	.dw 43
	.globl _gu2
_gu2:
	.db 3
	.db 5
	.db 6
	.dw 7
	.dw 8
	.db 4
	.ds 16
	.db 43
	.globl _gu3
_gu3:
	.dw 3
	.db 5
	.db 6
	.dw 7
	.dw 8
	.db 4
	.ds 16
	.db 43
	.globl _gu4
_gu4:
	.db 3
	.db 5
	.db 6
	.dw 7
	.db 5
	.ds 16
	.dw 44
	.globl _gs3
_gs3:
	.db 1
	.dw 2
	.db 3
	.dw 4
	.globl _gv
_gv:
	.dw 3
	.db 4
	.db 5
	.db 6
	.ds 17
	.db 0
	.dw 46
	.globl _gv2
_gv2:
	.dw 0
	.dw 0
	.ds 16
	.db 47
	.db 48
	.globl _gv3
_gv3:
	.dw 0
	.dw 0
	.ds 16
	.db 49
	.db 50
	.globl _gw
_gw:
	.dw 1
	.dw 0
	.ds 17
	.db 3
	.dw 4
	.dw 1
	.dw 0
	.dw 2
	.dw 0
	.dw 3
	.dw 0
	.dw 4
	.dw 0
	.dw 5
	.dw 0
	.globl _gsu
_gsu:
	.db 5
	.db 6
	.globl _guv
_guv:
	.dw 6
	.dw 5
	.globl _guv2
_guv2:
	.dw 7
	.dw 8
	.globl _guv3
_guv3:
	.db 8
	.db 7
	.globl _phdr
_phdr:
	.ds 16
	.ds 16
	.dw 4
	.dw 3
	.ds 16
	.ds 16
	.dw 7
	.dw 6
	.globl _global
_global:
	.ds 2
	.globl _global_wrap
_global_wrap:
	.dw 0
	.dw 0
	.globl _table
_table:
	.dw 0
	.dw 0
	.dw 0
	.dw 0
_test_correct_filling__i_0:
	.ds 2

	.area _CONST
__str_0:
	.db 37, 115, 58, 0
__str_4:
	.db 32, 37, 120, 0
__str_5:
	.db 10, 0
__str_6:
	.db 104, 101, 108, 108, 111, 0
__str_7:
	.db 104, 117, 104, 117, 0
__str_8:
	.db 104, 117, 104, 117, 0
__str_9:
	.db 104, 117, 104, 117, 0
__str_10:
	.db 104, 117, 104, 117, 0
__str_11:
	.db 104, 117, 104, 117, 0
__str_12:
	.db 98, 108, 97, 0
__str_13:
	.db 104, 97, 104, 97, 0
__str_14:
	.db 104, 105, 104, 105, 0
__str_15:
	.db 108, 115, 0
__str_16:
	.db 108, 115, 50, 0
__str_17:
	.db 108, 116, 0
__str_18:
	.db 108, 117, 0
__str_19:
	.db 108, 117, 49, 0
__str_20:
	.db 108, 117, 50, 0
__str_21:
	.db 108, 115, 50, 49, 0
__str_22:
	.db 108, 117, 50, 49, 0
__str_23:
	.db 108, 117, 50, 50, 0
__str_24:
	.db 108, 117, 51, 0
__str_25:
	.db 108, 117, 52, 0
__str_26:
	.db 108, 115, 51, 0
__str_27:
	.db 108, 118, 0
__str_28:
	.db 108, 118, 50, 0
__str_29:
	.db 108, 118, 51, 0
__str_30:
	.db 108, 116, 50, 0
__str_31:
	.db 102, 108, 111, 119, 0
__str_32:
	.db 110, 105, 10, 0
__str_33:
	.db 111, 110, 101, 10, 0
__str_34:
	.db 116, 119, 111, 10, 0
__str_35:
	.db 116, 104, 114, 101, 101, 10, 0
__str_52:
	.db 115, 101, 97, 95, 102, 105, 108, 108, 37, 100, 58, 32, 119, 114, 111, 110, 103, 10, 0
__str_53:
	.db 115, 101, 97, 95, 102, 105, 108, 108, 37, 100, 58, 32, 111, 107, 97, 121, 10, 0
__str_54:
	.db 99, 101, 0
__str_55:
	.db 103, 115, 0
__str_56:
	.db 103, 115, 50, 0
__str_57:
	.db 103, 116, 0
__str_58:
	.db 103, 117, 0
__str_59:
	.db 103, 117, 50, 0
__str_60:
	.db 103, 117, 51, 0
__str_61:
	.db 103, 117, 52, 0
__str_62:
	.db 103, 115, 51, 0
__str_63:
	.db 103, 118, 0
__str_64:
	.db 103, 118, 50, 0
__str_65:
	.db 103, 118, 51, 0
__str_66:
	.db 115, 105, 110, 105, 116, 49, 54, 0
__str_67:
	.db 103, 119, 0
__str_68:
	.db 103, 115, 117, 0
__str_69:
	.db 103, 117, 118, 0
__str_70:
	.db 103, 117, 118, 46, 98, 0
__str_71:
	.db 103, 117, 118, 50, 0
__str_72:
	.db 103, 117, 118, 51, 0
__str_73:
	.db 112, 104, 100, 114, 0


	.area _CODE

	.globl _inc_global
_inc_global:
	; prologue: inc_global (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, (_global)
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, (_global)
	inc	hl
	ld	(_global), hl
__inc_global_end:
	; epilogue: inc_global
	ld	sp, ix
	pop	ix
	ret
	.globl _print_
_print_:
	; prologue: print_ (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param name at 4(ix)
	; receive param p at 6(ix)
	; receive param size at 8(ix)
	ld	hl, #__str_0
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
__xcc_L1:
	ld	l, 8(ix)
	ld	h, 9(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, 10(ix)
	ld	h, 11(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, 8(ix)
	ld	h, 9(ix)
	push	hl
	ld	hl, #1
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	ld	8(ix), l
	ld	9(ix), h
	ld	l, 10(ix)
	ld	h, 11(ix)
	push	hl
	ld	hl, #0
	pop	de
	ex	de, hl
	sbc	hl, de
	ld	10(ix), l
	ld	11(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L2
	jp	__xcc_L3
__xcc_L2:
	ld	hl, #__str_4
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	inc	hl
	ld	6(ix), l
	ld	7(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	jp	__xcc_L1
__xcc_L3:
	ld	hl, #__str_5
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
__print__end:
	; epilogue: print_
	ld	sp, ix
	pop	ix
	ret
	.globl _foo
_foo:
	; prologue: foo (locals=319)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-319
	add	hl, sp
	ld	sp, hl
	; receive param w at 4(ix)
	; receive param phdr_ at 6(ix)
	push	ix
	pop	hl
	ld	de, #-4
	add	hl, de
	dec	sp
	dec	sp
	ld	-321(ix), l
	ld	-320(ix), h
	ld	l, -321(ix)
	ld	h, -320(ix)
	push	hl
	ld	de, #1
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -321(ix)
	ld	h, -320(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-323(ix), l
	ld	-322(ix), h
	ld	l, -323(ix)
	ld	h, -322(ix)
	push	hl
	ld	de, #2
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -321(ix)
	ld	h, -320(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-325(ix), l
	ld	-324(ix), h
	ld	l, -325(ix)
	ld	h, -324(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-8
	add	hl, de
	dec	sp
	dec	sp
	ld	-327(ix), l
	ld	-326(ix), h
	ld	l, -327(ix)
	ld	h, -326(ix)
	push	hl
	ld	de, #1
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -327(ix)
	ld	h, -326(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-329(ix), l
	ld	-328(ix), h
	ld	l, -329(ix)
	ld	h, -328(ix)
	push	hl
	ld	de, #2
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -327(ix)
	ld	h, -326(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-331(ix), l
	ld	-330(ix), h
	ld	l, -331(ix)
	ld	h, -330(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-25
	add	hl, de
	dec	sp
	dec	sp
	ld	-333(ix), l
	ld	-332(ix), h
	ld	hl, #__str_6
	dec	sp
	dec	sp
	ld	-335(ix), l
	ld	-334(ix), h
	ld	l, -333(ix)
	ld	h, -332(ix)
	push	hl
	ld	e, -335(ix)
	ld	d, -334(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -333(ix)
	ld	h, -332(ix)
	ld	de, #16
	add	hl, de
	dec	sp
	dec	sp
	ld	-337(ix), l
	ld	-336(ix), h
	ld	l, -337(ix)
	ld	h, -336(ix)
	push	hl
	ld	de, #42
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-48
	add	hl, de
	dec	sp
	dec	sp
	ld	-339(ix), l
	ld	-338(ix), h
	ld	l, -339(ix)
	ld	h, -338(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -339(ix)
	ld	h, -338(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-341(ix), l
	ld	-340(ix), h
	ld	l, -341(ix)
	ld	h, -340(ix)
	push	hl
	ld	de, #5
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -339(ix)
	ld	h, -338(ix)
	ld	de, #5
	add	hl, de
	dec	sp
	dec	sp
	ld	-343(ix), l
	ld	-342(ix), h
	ld	l, -343(ix)
	ld	h, -342(ix)
	push	hl
	ld	de, #6
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -339(ix)
	ld	h, -338(ix)
	ld	de, #6
	add	hl, de
	dec	sp
	dec	sp
	ld	-345(ix), l
	ld	-344(ix), h
	ld	l, -345(ix)
	ld	h, -344(ix)
	push	hl
	ld	de, #7
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-71
	add	hl, de
	dec	sp
	dec	sp
	ld	-347(ix), l
	ld	-346(ix), h
	ld	l, -347(ix)
	ld	h, -346(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -347(ix)
	ld	h, -346(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-349(ix), l
	ld	-348(ix), h
	ld	l, -349(ix)
	ld	h, -348(ix)
	push	hl
	ld	e, -4(ix)
	ld	d, -3(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -347(ix)
	ld	h, -346(ix)
	ld	de, #5
	add	hl, de
	dec	sp
	dec	sp
	ld	-351(ix), l
	ld	-350(ix), h
	ld	l, -351(ix)
	ld	h, -350(ix)
	push	hl
	ld	de, #4
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_7
	dec	sp
	dec	sp
	ld	-353(ix), l
	ld	-352(ix), h
	ld	l, -347(ix)
	ld	h, -346(ix)
	ld	de, #6
	add	hl, de
	dec	sp
	dec	sp
	ld	-355(ix), l
	ld	-354(ix), h
	ld	l, -355(ix)
	ld	h, -354(ix)
	push	hl
	ld	e, -353(ix)
	ld	d, -352(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-94
	add	hl, de
	dec	sp
	dec	sp
	ld	-357(ix), l
	ld	-356(ix), h
	ld	l, -357(ix)
	ld	h, -356(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -357(ix)
	ld	h, -356(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-359(ix), l
	ld	-358(ix), h
	ld	l, -359(ix)
	ld	h, -358(ix)
	push	hl
	ld	e, -4(ix)
	ld	d, -3(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -357(ix)
	ld	h, -356(ix)
	ld	de, #5
	add	hl, de
	dec	sp
	dec	sp
	ld	-361(ix), l
	ld	-360(ix), h
	ld	l, -361(ix)
	ld	h, -360(ix)
	push	hl
	ld	de, #4
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_8
	dec	sp
	dec	sp
	ld	-363(ix), l
	ld	-362(ix), h
	ld	l, -357(ix)
	ld	h, -356(ix)
	ld	de, #6
	add	hl, de
	dec	sp
	dec	sp
	ld	-365(ix), l
	ld	-364(ix), h
	ld	l, -365(ix)
	ld	h, -364(ix)
	push	hl
	ld	e, -363(ix)
	ld	d, -362(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-4
	add	hl, de
	dec	sp
	dec	sp
	ld	-367(ix), l
	ld	-366(ix), h
	ld	l, -367(ix)
	ld	h, -366(ix)
	ld	-96(ix), l
	ld	-95(ix), h
	ld	l, -96(ix)
	ld	h, -95(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-369(ix), l
	ld	-368(ix), h
	ld	l, -369(ix)
	ld	h, -368(ix)
	ld	-100(ix), l
	ld	-99(ix), h
	push	ix
	pop	hl
	ld	de, #-123
	add	hl, de
	dec	sp
	dec	sp
	ld	-371(ix), l
	ld	-370(ix), h
	ld	l, -371(ix)
	ld	h, -370(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -96(ix)
	ld	h, -95(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-373(ix), l
	ld	-372(ix), h
	ld	l, -371(ix)
	ld	h, -370(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-375(ix), l
	ld	-374(ix), h
	ld	l, -375(ix)
	ld	h, -374(ix)
	push	hl
	ld	e, -373(ix)
	ld	d, -372(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -371(ix)
	ld	h, -370(ix)
	ld	de, #5
	add	hl, de
	dec	sp
	dec	sp
	ld	-377(ix), l
	ld	-376(ix), h
	ld	l, -377(ix)
	ld	h, -376(ix)
	push	hl
	ld	de, #4
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_9
	dec	sp
	dec	sp
	ld	-379(ix), l
	ld	-378(ix), h
	ld	l, -371(ix)
	ld	h, -370(ix)
	ld	de, #6
	add	hl, de
	dec	sp
	dec	sp
	ld	-381(ix), l
	ld	-380(ix), h
	ld	l, -381(ix)
	ld	h, -380(ix)
	push	hl
	ld	e, -379(ix)
	ld	d, -378(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-146
	add	hl, de
	dec	sp
	dec	sp
	ld	-383(ix), l
	ld	-382(ix), h
	ld	l, -383(ix)
	ld	h, -382(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -383(ix)
	ld	h, -382(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-385(ix), l
	ld	-384(ix), h
	ld	l, -385(ix)
	ld	h, -384(ix)
	push	hl
	ld	e, -4(ix)
	ld	d, -3(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -383(ix)
	ld	h, -382(ix)
	ld	de, #5
	add	hl, de
	dec	sp
	dec	sp
	ld	-387(ix), l
	ld	-386(ix), h
	ld	l, -387(ix)
	ld	h, -386(ix)
	push	hl
	ld	de, #4
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_10
	dec	sp
	dec	sp
	ld	-389(ix), l
	ld	-388(ix), h
	ld	l, -383(ix)
	ld	h, -382(ix)
	ld	de, #6
	add	hl, de
	dec	sp
	dec	sp
	ld	-391(ix), l
	ld	-390(ix), h
	ld	l, -391(ix)
	ld	h, -390(ix)
	push	hl
	ld	e, -389(ix)
	ld	d, -388(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-169
	add	hl, de
	dec	sp
	dec	sp
	ld	-393(ix), l
	ld	-392(ix), h
	ld	l, -393(ix)
	ld	h, -392(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -393(ix)
	ld	h, -392(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-395(ix), l
	ld	-394(ix), h
	ld	l, -395(ix)
	ld	h, -394(ix)
	push	hl
	ld	de, #5
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -393(ix)
	ld	h, -392(ix)
	ld	de, #5
	add	hl, de
	dec	sp
	dec	sp
	ld	-397(ix), l
	ld	-396(ix), h
	ld	l, -397(ix)
	ld	h, -396(ix)
	push	hl
	ld	de, #4
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_11
	dec	sp
	dec	sp
	ld	-399(ix), l
	ld	-398(ix), h
	ld	l, -393(ix)
	ld	h, -392(ix)
	ld	de, #6
	add	hl, de
	dec	sp
	dec	sp
	ld	-401(ix), l
	ld	-400(ix), h
	ld	l, -401(ix)
	ld	h, -400(ix)
	push	hl
	ld	e, -399(ix)
	ld	d, -398(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-192
	add	hl, de
	dec	sp
	dec	sp
	ld	-403(ix), l
	ld	-402(ix), h
	ld	l, -403(ix)
	ld	h, -402(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -403(ix)
	ld	h, -402(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-405(ix), l
	ld	-404(ix), h
	ld	l, -405(ix)
	ld	h, -404(ix)
	push	hl
	ld	de, #5
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -403(ix)
	ld	h, -402(ix)
	ld	de, #5
	add	hl, de
	dec	sp
	dec	sp
	ld	-407(ix), l
	ld	-406(ix), h
	ld	l, -407(ix)
	ld	h, -406(ix)
	push	hl
	ld	de, #5
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_12
	dec	sp
	dec	sp
	ld	-409(ix), l
	ld	-408(ix), h
	ld	l, -403(ix)
	ld	h, -402(ix)
	ld	de, #6
	add	hl, de
	dec	sp
	dec	sp
	ld	-411(ix), l
	ld	-410(ix), h
	ld	l, -411(ix)
	ld	h, -410(ix)
	push	hl
	ld	e, -409(ix)
	ld	d, -408(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-196
	add	hl, de
	dec	sp
	dec	sp
	ld	-413(ix), l
	ld	-412(ix), h
	ld	l, -413(ix)
	ld	h, -412(ix)
	push	hl
	ld	de, #1
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -413(ix)
	ld	h, -412(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-415(ix), l
	ld	-414(ix), h
	ld	l, -415(ix)
	ld	h, -414(ix)
	push	hl
	ld	de, #2
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -413(ix)
	ld	h, -412(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-417(ix), l
	ld	-416(ix), h
	ld	l, -417(ix)
	ld	h, -416(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-218
	add	hl, de
	dec	sp
	dec	sp
	ld	-419(ix), l
	ld	-418(ix), h
	ld	l, -419(ix)
	ld	h, -418(ix)
	push	hl
	ld	de, #3
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_13
	dec	sp
	dec	sp
	ld	-421(ix), l
	ld	-420(ix), h
	ld	l, -419(ix)
	ld	h, -418(ix)
	ld	de, #4
	add	hl, de
	dec	sp
	dec	sp
	ld	-423(ix), l
	ld	-422(ix), h
	ld	l, -423(ix)
	ld	h, -422(ix)
	push	hl
	ld	e, -421(ix)
	ld	d, -420(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #45
	ld	a, l
	dec	sp
	dec	sp
	ld	-425(ix), a
	ld	l, -419(ix)
	ld	h, -418(ix)
	ld	de, #21
	add	hl, de
	dec	sp
	dec	sp
	ld	-427(ix), l
	ld	-426(ix), h
	ld	l, -427(ix)
	ld	h, -426(ix)
	push	hl
	ld	a, -425(ix)
	pop	hl
	ld	(hl), a
	push	ix
	pop	hl
	ld	de, #-240
	add	hl, de
	dec	sp
	dec	sp
	ld	-429(ix), l
	ld	-428(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-431(ix), l
	ld	-430(ix), h
	push	ix
	pop	hl
	ld	de, #-431
	add	hl, de
	dec	sp
	dec	sp
	ld	-433(ix), l
	ld	-432(ix), h
	ld	l, -433(ix)
	ld	h, -432(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-435(ix), l
	ld	-434(ix), h
	ld	l, -435(ix)
	ld	h, -434(ix)
	dec	sp
	dec	sp
	dec	sp
	dec	sp
	ld	-439(ix), l
	ld	-438(ix), h
	ld	l, -429(ix)
	ld	h, -428(ix)
	push	hl
	ld	e, -439(ix)
	ld	d, -438(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_14
	dec	sp
	dec	sp
	ld	-441(ix), l
	ld	-440(ix), h
	ld	l, -429(ix)
	ld	h, -428(ix)
	ld	de, #4
	add	hl, de
	dec	sp
	dec	sp
	ld	-443(ix), l
	ld	-442(ix), h
	ld	l, -443(ix)
	ld	h, -442(ix)
	push	hl
	ld	e, -441(ix)
	ld	d, -440(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -429(ix)
	ld	h, -428(ix)
	ld	de, #21
	add	hl, de
	dec	sp
	dec	sp
	ld	-445(ix), l
	ld	-444(ix), h
	ld	l, -445(ix)
	ld	h, -444(ix)
	push	hl
	ld	de, #48
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-262
	add	hl, de
	dec	sp
	dec	sp
	ld	-447(ix), l
	ld	-446(ix), h
	push	ix
	pop	hl
	ld	de, #-266
	add	hl, de
	dec	sp
	dec	sp
	ld	-449(ix), l
	ld	-448(ix), h
	ld	l, -449(ix)
	ld	h, -448(ix)
	push	hl
	ld	de, #7
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -449(ix)
	ld	h, -448(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-451(ix), l
	ld	-450(ix), h
	ld	l, -451(ix)
	ld	h, -450(ix)
	push	hl
	ld	de, #8
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -449(ix)
	ld	h, -448(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-453(ix), l
	ld	-452(ix), h
	ld	l, -453(ix)
	ld	h, -452(ix)
	push	hl
	ld	de, #9
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -447(ix)
	ld	h, -446(ix)
	push	hl
	ld	e, -266(ix)
	ld	d, -265(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, 4(ix)
	ld	h, 5(ix)
	dec	sp
	dec	sp
	ld	-455(ix), l
	ld	-454(ix), h
	ld	l, -455(ix)
	ld	h, -454(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-457(ix), l
	ld	-456(ix), h
	push	ix
	pop	hl
	ld	de, #-457
	add	hl, de
	dec	sp
	dec	sp
	ld	-459(ix), l
	ld	-458(ix), h
	ld	l, -459(ix)
	ld	h, -458(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-461(ix), l
	ld	-460(ix), h
	ld	l, -447(ix)
	ld	h, -446(ix)
	ld	de, #4
	add	hl, de
	dec	sp
	dec	sp
	ld	-463(ix), l
	ld	-462(ix), h
	ld	l, -463(ix)
	ld	h, -462(ix)
	push	hl
	ld	e, -461(ix)
	ld	d, -460(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -447(ix)
	ld	h, -446(ix)
	ld	de, #21
	add	hl, de
	dec	sp
	dec	sp
	ld	-465(ix), l
	ld	-464(ix), h
	ld	l, -465(ix)
	ld	h, -464(ix)
	push	hl
	ld	de, #50
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, 6(ix)
	ld	h, 7(ix)
	ld	-268(ix), l
	ld	-267(ix), h
	push	ix
	pop	hl
	ld	de, #-300
	add	hl, de
	dec	sp
	dec	sp
	ld	-467(ix), l
	ld	-466(ix), h
	ld	l, -268(ix)
	ld	h, -267(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-469(ix), l
	ld	-468(ix), h
	ld	l, -467(ix)
	ld	h, -466(ix)
	ld	de, #16
	add	hl, de
	dec	sp
	dec	sp
	ld	-471(ix), l
	ld	-470(ix), h
	ld	l, -471(ix)
	ld	h, -470(ix)
	push	hl
	ld	e, -469(ix)
	ld	d, -468(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -268(ix)
	ld	h, -267(ix)
	ld	de, #16
	add	hl, de
	dec	sp
	dec	sp
	ld	-473(ix), l
	ld	-472(ix), h
	ld	l, -473(ix)
	ld	h, -472(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-475(ix), l
	ld	-474(ix), h
	ld	l, -467(ix)
	ld	h, -466(ix)
	push	hl
	ld	e, -475(ix)
	ld	d, -474(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #66
	ld	-302(ix), l
	ld	-301(ix), h
	push	ix
	pop	hl
	ld	de, #-319
	add	hl, de
	dec	sp
	dec	sp
	ld	-477(ix), l
	ld	-476(ix), h
	ld	l, -477(ix)
	ld	h, -476(ix)
	push	hl
	ld	de, #9
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -477(ix)
	ld	h, -476(ix)
	ld	de, #16
	add	hl, de
	dec	sp
	dec	sp
	ld	-479(ix), l
	ld	-478(ix), h
	ld	l, -479(ix)
	ld	h, -478(ix)
	push	hl
	ld	de, #1
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #__str_15
	dec	sp
	dec	sp
	ld	-481(ix), l
	ld	-480(ix), h
	push	ix
	pop	hl
	ld	de, #-4
	add	hl, de
	dec	sp
	dec	sp
	ld	-483(ix), l
	ld	-482(ix), h
	ld	l, -483(ix)
	ld	h, -482(ix)
	dec	sp
	dec	sp
	ld	-485(ix), l
	ld	-484(ix), h
	ld	hl, #4
	push	hl
	ld	l, -485(ix)
	ld	h, -484(ix)
	push	hl
	ld	l, -481(ix)
	ld	h, -480(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_16
	dec	sp
	dec	sp
	ld	-487(ix), l
	ld	-486(ix), h
	push	ix
	pop	hl
	ld	de, #-8
	add	hl, de
	dec	sp
	dec	sp
	ld	-489(ix), l
	ld	-488(ix), h
	ld	l, -489(ix)
	ld	h, -488(ix)
	dec	sp
	dec	sp
	ld	-491(ix), l
	ld	-490(ix), h
	ld	hl, #4
	push	hl
	ld	l, -491(ix)
	ld	h, -490(ix)
	push	hl
	ld	l, -487(ix)
	ld	h, -486(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_17
	dec	sp
	dec	sp
	ld	-493(ix), l
	ld	-492(ix), h
	push	ix
	pop	hl
	ld	de, #-25
	add	hl, de
	dec	sp
	dec	sp
	ld	-495(ix), l
	ld	-494(ix), h
	ld	l, -495(ix)
	ld	h, -494(ix)
	dec	sp
	dec	sp
	ld	-497(ix), l
	ld	-496(ix), h
	ld	hl, #17
	push	hl
	ld	l, -497(ix)
	ld	h, -496(ix)
	push	hl
	ld	l, -493(ix)
	ld	h, -492(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_18
	dec	sp
	dec	sp
	ld	-499(ix), l
	ld	-498(ix), h
	push	ix
	pop	hl
	ld	de, #-48
	add	hl, de
	dec	sp
	dec	sp
	ld	-501(ix), l
	ld	-500(ix), h
	ld	l, -501(ix)
	ld	h, -500(ix)
	dec	sp
	dec	sp
	ld	-503(ix), l
	ld	-502(ix), h
	ld	hl, #23
	push	hl
	ld	l, -503(ix)
	ld	h, -502(ix)
	push	hl
	ld	l, -499(ix)
	ld	h, -498(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_19
	dec	sp
	dec	sp
	ld	-505(ix), l
	ld	-504(ix), h
	push	ix
	pop	hl
	ld	de, #-71
	add	hl, de
	dec	sp
	dec	sp
	ld	-507(ix), l
	ld	-506(ix), h
	ld	l, -507(ix)
	ld	h, -506(ix)
	dec	sp
	dec	sp
	ld	-509(ix), l
	ld	-508(ix), h
	ld	hl, #23
	push	hl
	ld	l, -509(ix)
	ld	h, -508(ix)
	push	hl
	ld	l, -505(ix)
	ld	h, -504(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_20
	dec	sp
	dec	sp
	ld	-511(ix), l
	ld	-510(ix), h
	push	ix
	pop	hl
	ld	de, #-94
	add	hl, de
	dec	sp
	dec	sp
	ld	-513(ix), l
	ld	-512(ix), h
	ld	l, -513(ix)
	ld	h, -512(ix)
	dec	sp
	dec	sp
	ld	-515(ix), l
	ld	-514(ix), h
	ld	hl, #23
	push	hl
	ld	l, -515(ix)
	ld	h, -514(ix)
	push	hl
	ld	l, -511(ix)
	ld	h, -510(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_21
	dec	sp
	dec	sp
	ld	-517(ix), l
	ld	-516(ix), h
	push	ix
	pop	hl
	ld	de, #-100
	add	hl, de
	dec	sp
	dec	sp
	ld	-519(ix), l
	ld	-518(ix), h
	ld	l, -519(ix)
	ld	h, -518(ix)
	dec	sp
	dec	sp
	ld	-521(ix), l
	ld	-520(ix), h
	ld	hl, #4
	push	hl
	ld	l, -521(ix)
	ld	h, -520(ix)
	push	hl
	ld	l, -517(ix)
	ld	h, -516(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_22
	dec	sp
	dec	sp
	ld	-523(ix), l
	ld	-522(ix), h
	push	ix
	pop	hl
	ld	de, #-146
	add	hl, de
	dec	sp
	dec	sp
	ld	-525(ix), l
	ld	-524(ix), h
	ld	l, -525(ix)
	ld	h, -524(ix)
	dec	sp
	dec	sp
	ld	-527(ix), l
	ld	-526(ix), h
	ld	hl, #23
	push	hl
	ld	l, -527(ix)
	ld	h, -526(ix)
	push	hl
	ld	l, -523(ix)
	ld	h, -522(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_23
	dec	sp
	dec	sp
	ld	-529(ix), l
	ld	-528(ix), h
	push	ix
	pop	hl
	ld	de, #-123
	add	hl, de
	dec	sp
	dec	sp
	ld	-531(ix), l
	ld	-530(ix), h
	ld	l, -531(ix)
	ld	h, -530(ix)
	dec	sp
	dec	sp
	ld	-533(ix), l
	ld	-532(ix), h
	ld	hl, #23
	push	hl
	ld	l, -533(ix)
	ld	h, -532(ix)
	push	hl
	ld	l, -529(ix)
	ld	h, -528(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_24
	dec	sp
	dec	sp
	ld	-535(ix), l
	ld	-534(ix), h
	push	ix
	pop	hl
	ld	de, #-169
	add	hl, de
	dec	sp
	dec	sp
	ld	-537(ix), l
	ld	-536(ix), h
	ld	l, -537(ix)
	ld	h, -536(ix)
	dec	sp
	dec	sp
	ld	-539(ix), l
	ld	-538(ix), h
	ld	hl, #23
	push	hl
	ld	l, -539(ix)
	ld	h, -538(ix)
	push	hl
	ld	l, -535(ix)
	ld	h, -534(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_25
	dec	sp
	dec	sp
	ld	-541(ix), l
	ld	-540(ix), h
	push	ix
	pop	hl
	ld	de, #-192
	add	hl, de
	dec	sp
	dec	sp
	ld	-543(ix), l
	ld	-542(ix), h
	ld	l, -543(ix)
	ld	h, -542(ix)
	dec	sp
	dec	sp
	ld	-545(ix), l
	ld	-544(ix), h
	ld	hl, #23
	push	hl
	ld	l, -545(ix)
	ld	h, -544(ix)
	push	hl
	ld	l, -541(ix)
	ld	h, -540(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_26
	dec	sp
	dec	sp
	ld	-547(ix), l
	ld	-546(ix), h
	push	ix
	pop	hl
	ld	de, #-196
	add	hl, de
	dec	sp
	dec	sp
	ld	-549(ix), l
	ld	-548(ix), h
	ld	l, -549(ix)
	ld	h, -548(ix)
	dec	sp
	dec	sp
	ld	-551(ix), l
	ld	-550(ix), h
	ld	hl, #4
	push	hl
	ld	l, -551(ix)
	ld	h, -550(ix)
	push	hl
	ld	l, -547(ix)
	ld	h, -546(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_27
	dec	sp
	dec	sp
	ld	-553(ix), l
	ld	-552(ix), h
	push	ix
	pop	hl
	ld	de, #-218
	add	hl, de
	dec	sp
	dec	sp
	ld	-555(ix), l
	ld	-554(ix), h
	ld	l, -555(ix)
	ld	h, -554(ix)
	dec	sp
	dec	sp
	ld	-557(ix), l
	ld	-556(ix), h
	ld	hl, #22
	push	hl
	ld	l, -557(ix)
	ld	h, -556(ix)
	push	hl
	ld	l, -553(ix)
	ld	h, -552(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_28
	dec	sp
	dec	sp
	ld	-559(ix), l
	ld	-558(ix), h
	push	ix
	pop	hl
	ld	de, #-240
	add	hl, de
	dec	sp
	dec	sp
	ld	-561(ix), l
	ld	-560(ix), h
	ld	l, -561(ix)
	ld	h, -560(ix)
	dec	sp
	dec	sp
	ld	-563(ix), l
	ld	-562(ix), h
	ld	hl, #22
	push	hl
	ld	l, -563(ix)
	ld	h, -562(ix)
	push	hl
	ld	l, -559(ix)
	ld	h, -558(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_29
	dec	sp
	dec	sp
	ld	-565(ix), l
	ld	-564(ix), h
	push	ix
	pop	hl
	ld	de, #-262
	add	hl, de
	dec	sp
	dec	sp
	ld	-567(ix), l
	ld	-566(ix), h
	ld	l, -567(ix)
	ld	h, -566(ix)
	dec	sp
	dec	sp
	ld	-569(ix), l
	ld	-568(ix), h
	ld	hl, #22
	push	hl
	ld	l, -569(ix)
	ld	h, -568(ix)
	push	hl
	ld	l, -565(ix)
	ld	h, -564(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_30
	dec	sp
	dec	sp
	ld	-571(ix), l
	ld	-570(ix), h
	push	ix
	pop	hl
	ld	de, #-319
	add	hl, de
	dec	sp
	dec	sp
	ld	-573(ix), l
	ld	-572(ix), h
	ld	l, -573(ix)
	ld	h, -572(ix)
	dec	sp
	dec	sp
	ld	-575(ix), l
	ld	-574(ix), h
	ld	hl, #17
	push	hl
	ld	l, -575(ix)
	ld	h, -574(ix)
	push	hl
	ld	l, -571(ix)
	ld	h, -570(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_31
	dec	sp
	dec	sp
	ld	-577(ix), l
	ld	-576(ix), h
	push	ix
	pop	hl
	ld	de, #-300
	add	hl, de
	dec	sp
	dec	sp
	ld	-579(ix), l
	ld	-578(ix), h
	ld	l, -579(ix)
	ld	h, -578(ix)
	dec	sp
	dec	sp
	ld	-581(ix), l
	ld	-580(ix), h
	ld	hl, #32
	push	hl
	ld	l, -581(ix)
	ld	h, -580(ix)
	push	hl
	ld	l, -577(ix)
	ld	h, -576(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
__foo_end:
	; epilogue: foo
	ld	sp, ix
	pop	ix
	ret
	.globl _test_compound_with_relocs
_test_compound_with_relocs:
	; prologue: test_compound_with_relocs (locals=4)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	push	ix
	pop	hl
	ld	de, #0
	add	hl, de
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	push	ix
	pop	hl
	ld	de, #-2
	add	hl, de
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	hl, #_inc_global
	ex	de, hl
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	e, -2(ix)
	ld	d, -1(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	hl, #_inc_global
	ex	de, hl
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
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, (_global_wrap)
	ld	e, -12(ix)
	ld	d, -11(ix)
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
	push	ix
	pop	hl
	ld	de, #-16
	add	hl, de
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	.globl __call_hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	call	__call_hl
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	hl, (_global_wrap)
	ld	e, -22(ix)
	ld	d, -21(ix)
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
	ld	-4(ix), l
	ld	-3(ix), h
	.globl __call_hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	call	__call_hl
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
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	e, -32(ix)
	ld	d, -31(ix)
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
	ld	-4(ix), l
	ld	-3(ix), h
	.globl __call_hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	call	__call_hl
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #1
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, 0(ix)
	ld	h, 1(ix)
	ld	e, -42(ix)
	ld	d, -41(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	push	ix
	pop	hl
	ld	de, #-46
	add	hl, de
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	l, -48(ix)
	ld	h, -47(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	.globl __call_hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	call	__call_hl
__test_compound_with_relocs_end:
	; epilogue: test_compound_with_relocs
	ld	sp, ix
	pop	ix
	ret
	.globl _sys_ni
_sys_ni:
	; prologue: sys_ni (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_32
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
__sys_ni_end:
	; epilogue: sys_ni
	ld	sp, ix
	pop	ix
	ret
	.globl _sys_one
_sys_one:
	; prologue: sys_one (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_33
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
__sys_one_end:
	; epilogue: sys_one
	ld	sp, ix
	pop	ix
	ret
	.globl _sys_two
_sys_two:
	; prologue: sys_two (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_34
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
__sys_two_end:
	; epilogue: sys_two
	ld	sp, ix
	pop	ix
	ret
	.globl _sys_three
_sys_three:
	; prologue: sys_three (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_35
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
__sys_three_end:
	; epilogue: sys_three
	ld	sp, ix
	pop	ix
	ret
	.globl _test_multi_relocs
_test_multi_relocs:
	; prologue: test_multi_relocs (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L36:
	ld	hl, #0
	push	hl
	ld	hl, #6
	pop	de
	.globl __divsint
	call	__divsint
	ex	de, hl
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L37
	jp	__xcc_L39
__xcc_L37:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, (_table)
	ld	e, -8(ix)
	ld	d, -7(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	.globl __call_hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	call	__call_hl
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
__xcc_L38:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L36
__xcc_L39:
__test_multi_relocs_end:
	; epilogue: test_multi_relocs
	ld	sp, ix
	pop	ix
	ret
_test_correct_filling:
	; prologue: test_correct_filling (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param x at 4(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #0
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
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	hl, #0
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
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L47
	jp	__xcc_L48
__xcc_L48:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	de, #2
	add	hl, de
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	hl, #5
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
	ld	-12(ix), l
	ld	-11(ix), h
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	hl, #0
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
	ld	-14(ix), l
	ld	-13(ix), h
	jp	__xcc_L49
__xcc_L47:
	ld	hl, #1
	ld	-14(ix), l
	ld	-13(ix), h
__xcc_L49:
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_2362
	ld	hl, #0
	jp	__cmp_e_90027
__cmp_t_2362:
	ld	hl, #1
__cmp_e_90027:
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L45
	jp	__xcc_L46
__xcc_L46:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	de, #4
	add	hl, de
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_68690
	ld	hl, #0
	jp	__cmp_e_20059
__cmp_t_68690:
	ld	hl, #1
__cmp_e_20059:
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_97763
	ld	hl, #0
	jp	__cmp_e_13926
__cmp_t_97763:
	ld	hl, #1
__cmp_e_13926:
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	jp	__xcc_L50
__xcc_L45:
	ld	hl, #1
	ld	-24(ix), l
	ld	-23(ix), h
__xcc_L50:
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_80540
	ld	hl, #0
	jp	__cmp_e_83426
__cmp_t_80540:
	ld	hl, #1
__cmp_e_83426:
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L43
	jp	__xcc_L44
__xcc_L44:
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	de, #6
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
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_89172
	ld	hl, #0
	jp	__cmp_e_55736
__cmp_t_89172:
	ld	hl, #1
__cmp_e_55736:
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	ld	hl, #0
	pop	de
	or	a, a
	sbc	hl, de
	jp	nz, __cmp_t_5211
	ld	hl, #0
	jp	__cmp_e_95368
__cmp_t_5211:
	ld	hl, #1
__cmp_e_95368:
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	jp	__xcc_L51
__xcc_L43:
	ld	hl, #1
	ld	-34(ix), l
	ld	-33(ix), h
__xcc_L51:
	ld	l, -34(ix)
	ld	h, -33(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L40
	jp	__xcc_L41
__xcc_L40:
	ld	hl, #__str_52
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	hl, (_test_correct_filling__i_0)
	push	hl
	ld	l, -36(ix)
	ld	h, -35(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	jp	__xcc_L42
__xcc_L41:
	ld	hl, #__str_53
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	hl, (_test_correct_filling__i_0)
	push	hl
	ld	l, -40(ix)
	ld	h, -39(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
__xcc_L42:
	ld	hl, (_test_correct_filling__i_0)
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	hl, (_test_correct_filling__i_0)
	inc	hl
	ld	(_test_correct_filling__i_0), hl
__test_correct_filling_end:
	; epilogue: test_correct_filling
	ld	sp, ix
	pop	ix
	ret
	.globl _test_zero_init
_test_zero_init:
	; prologue: test_zero_init (locals=26)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-26
	add	hl, sp
	ld	sp, hl
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
	push	hl
	ld	de, #5
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-18
	add	hl, de
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	ld	de, #5
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	push	ix
	pop	hl
	ld	de, #-26
	add	hl, de
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	ld	de, #5
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
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
	push	hl
	.globl _test_correct_filling
	call	_test_correct_filling
	pop	bc
	push	ix
	pop	hl
	ld	de, #-18
	add	hl, de
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	push	ix
	pop	hl
	ld	de, #-42
	add	hl, de
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	l, -44(ix)
	ld	h, -43(ix)
	push	hl
	.globl _test_correct_filling
	call	_test_correct_filling
	pop	bc
	push	ix
	pop	hl
	ld	de, #-26
	add	hl, de
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	push	ix
	pop	hl
	ld	de, #-48
	add	hl, de
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	.globl _test_correct_filling
	call	_test_correct_filling
	pop	bc
	ld	hl, #0
	jp	__test_zero_init_end
__test_zero_init_end:
	; epilogue: test_zero_init
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #__str_54
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #_ce
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	hl, #2
	push	hl
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_55
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, #_gs
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, #4
	push	hl
	ld	l, -12(ix)
	ld	h, -11(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_56
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	hl, #_gs2
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	hl, #4
	push	hl
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_57
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	hl, #_gt
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, #17
	push	hl
	ld	l, -24(ix)
	ld	h, -23(ix)
	push	hl
	ld	l, -20(ix)
	ld	h, -19(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_58
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	hl, #_gu
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -28(ix)
	ld	h, -27(ix)
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	hl, #23
	push	hl
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_59
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	hl, #_gu2
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	hl, #23
	push	hl
	ld	l, -36(ix)
	ld	h, -35(ix)
	push	hl
	ld	l, -32(ix)
	ld	h, -31(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_60
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	hl, #_gu3
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	l, -40(ix)
	ld	h, -39(ix)
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	hl, #23
	push	hl
	ld	l, -42(ix)
	ld	h, -41(ix)
	push	hl
	ld	l, -38(ix)
	ld	h, -37(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_61
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	hl, #_gu4
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	hl, #23
	push	hl
	ld	l, -48(ix)
	ld	h, -47(ix)
	push	hl
	ld	l, -44(ix)
	ld	h, -43(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_62
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	hl, #_gs3
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	l, -52(ix)
	ld	h, -51(ix)
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	hl, #4
	push	hl
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_63
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	hl, #_gv
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, -58(ix)
	ld	h, -57(ix)
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	ld	hl, #22
	push	hl
	ld	l, -60(ix)
	ld	h, -59(ix)
	push	hl
	ld	l, -56(ix)
	ld	h, -55(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_64
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	hl, #_gv2
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	l, -64(ix)
	ld	h, -63(ix)
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	hl, #22
	push	hl
	ld	l, -66(ix)
	ld	h, -65(ix)
	push	hl
	ld	l, -62(ix)
	ld	h, -61(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_65
	dec	sp
	dec	sp
	ld	-68(ix), l
	ld	-67(ix), h
	ld	hl, #_gv3
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
	ld	l, -70(ix)
	ld	h, -69(ix)
	dec	sp
	dec	sp
	ld	-72(ix), l
	ld	-71(ix), h
	ld	hl, #22
	push	hl
	ld	l, -72(ix)
	ld	h, -71(ix)
	push	hl
	ld	l, -68(ix)
	ld	h, -67(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_66
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	ld	hl, #_sinit16
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	l, -76(ix)
	ld	h, -75(ix)
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
	ld	hl, #0
	push	hl
	ld	l, -78(ix)
	ld	h, -77(ix)
	push	hl
	ld	l, -74(ix)
	ld	h, -73(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_67
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	ld	hl, #_gw
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	l, -82(ix)
	ld	h, -81(ix)
	dec	sp
	dec	sp
	ld	-84(ix), l
	ld	-83(ix), h
	ld	hl, #22
	push	hl
	ld	l, -84(ix)
	ld	h, -83(ix)
	push	hl
	ld	l, -80(ix)
	ld	h, -79(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_68
	dec	sp
	dec	sp
	ld	-86(ix), l
	ld	-85(ix), h
	ld	hl, #_gsu
	dec	sp
	dec	sp
	ld	-88(ix), l
	ld	-87(ix), h
	ld	l, -88(ix)
	ld	h, -87(ix)
	dec	sp
	dec	sp
	ld	-90(ix), l
	ld	-89(ix), h
	ld	hl, #2
	push	hl
	ld	l, -90(ix)
	ld	h, -89(ix)
	push	hl
	ld	l, -86(ix)
	ld	h, -85(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_69
	dec	sp
	dec	sp
	ld	-92(ix), l
	ld	-91(ix), h
	ld	hl, #_guv
	dec	sp
	dec	sp
	ld	-94(ix), l
	ld	-93(ix), h
	ld	l, -94(ix)
	ld	h, -93(ix)
	dec	sp
	dec	sp
	ld	-96(ix), l
	ld	-95(ix), h
	ld	hl, #4
	push	hl
	ld	l, -96(ix)
	ld	h, -95(ix)
	push	hl
	ld	l, -92(ix)
	ld	h, -91(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_70
	dec	sp
	dec	sp
	ld	-98(ix), l
	ld	-97(ix), h
	ld	hl, #_guv
	dec	sp
	dec	sp
	ld	-100(ix), l
	ld	-99(ix), h
	ld	l, -100(ix)
	ld	h, -99(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-102(ix), l
	ld	-101(ix), h
	ld	l, -102(ix)
	ld	h, -101(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-104(ix), l
	ld	-103(ix), h
	push	ix
	pop	hl
	ld	de, #-104
	add	hl, de
	dec	sp
	dec	sp
	ld	-106(ix), l
	ld	-105(ix), h
	ld	l, -106(ix)
	ld	h, -105(ix)
	dec	sp
	dec	sp
	ld	-108(ix), l
	ld	-107(ix), h
	ld	hl, #0
	push	hl
	ld	l, -108(ix)
	ld	h, -107(ix)
	push	hl
	ld	l, -98(ix)
	ld	h, -97(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_71
	dec	sp
	dec	sp
	ld	-110(ix), l
	ld	-109(ix), h
	ld	hl, #_guv2
	dec	sp
	dec	sp
	ld	-112(ix), l
	ld	-111(ix), h
	ld	l, -112(ix)
	ld	h, -111(ix)
	dec	sp
	dec	sp
	ld	-114(ix), l
	ld	-113(ix), h
	ld	hl, #4
	push	hl
	ld	l, -114(ix)
	ld	h, -113(ix)
	push	hl
	ld	l, -110(ix)
	ld	h, -109(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_72
	dec	sp
	dec	sp
	ld	-116(ix), l
	ld	-115(ix), h
	ld	hl, #_guv3
	dec	sp
	dec	sp
	ld	-118(ix), l
	ld	-117(ix), h
	ld	l, -118(ix)
	ld	h, -117(ix)
	dec	sp
	dec	sp
	ld	-120(ix), l
	ld	-119(ix), h
	ld	hl, #4
	push	hl
	ld	l, -120(ix)
	ld	h, -119(ix)
	push	hl
	ld	l, -116(ix)
	ld	h, -115(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #__str_73
	dec	sp
	dec	sp
	ld	-122(ix), l
	ld	-121(ix), h
	ld	hl, #_phdr
	dec	sp
	dec	sp
	ld	-124(ix), l
	ld	-123(ix), h
	ld	l, -124(ix)
	ld	h, -123(ix)
	dec	sp
	dec	sp
	ld	-126(ix), l
	ld	-125(ix), h
	ld	hl, #32
	push	hl
	ld	l, -126(ix)
	ld	h, -125(ix)
	push	hl
	ld	l, -122(ix)
	ld	h, -121(ix)
	push	hl
	.globl _print_
	call	_print_
	pop	bc
	pop	bc
	pop	bc
	ld	hl, #_gw
	dec	sp
	dec	sp
	ld	-128(ix), l
	ld	-127(ix), h
	ld	hl, #_phdr
	dec	sp
	dec	sp
	ld	-130(ix), l
	ld	-129(ix), h
	ld	l, -130(ix)
	ld	h, -129(ix)
	push	hl
	ld	l, -128(ix)
	ld	h, -127(ix)
	push	hl
	.globl _foo
	call	_foo
	pop	bc
	pop	bc
	.globl _test_compound_with_relocs
	call	_test_compound_with_relocs
	.globl _test_multi_relocs
	call	_test_multi_relocs
	.globl _test_zero_init
	call	_test_zero_init
	dec	sp
	dec	sp
	ld	-132(ix), l
	ld	-131(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
