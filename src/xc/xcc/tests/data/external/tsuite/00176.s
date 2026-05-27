	.module xcc_output

	.area _DATA
	.globl _array
_array:
	.ds 32

	.area _CONST
__str_14:
	.db 37, 100, 32, 0
__str_15:
	.db 10, 0
__str_20:
	.db 37, 100, 32, 0
__str_21:
	.db 10, 0


	.area _CODE

	.globl _swap
_swap:
	; prologue: swap (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	; receive param a at 4(ix)
	; receive param b at 6(ix)
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, (_array)
	ld	e, -4(ix)
	ld	d, -3(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	hl, (_array)
	ld	e, -10(ix)
	ld	d, -9(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
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
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, (_array)
	ld	e, -16(ix)
	ld	d, -15(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	ld	e, -14(ix)
	ld	d, -13(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	hl, (_array)
	ld	e, -20(ix)
	ld	d, -19(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	ld	e, -2(ix)
	ld	d, -1(ix)
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
__swap_end:
	; epilogue: swap
	ld	sp, ix
	pop	ix
	ret
	.globl _partition
_partition:
	; prologue: partition (locals=8)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-8
	add	hl, sp
	ld	sp, hl
	; receive param left at 4(ix)
	; receive param right at 6(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	-2(ix), l
	ld	-1(ix), h
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
	ld	-10(ix), l
	ld	-9(ix), h
	ld	hl, (_array)
	ld	e, -10(ix)
	ld	d, -9(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
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
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	.globl _swap
	call	_swap
	pop	bc
	pop	bc
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	-8(ix), l
	ld	-7(ix), h
__xcc_L0:
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
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
	ld	-16(ix), l
	ld	-15(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L1
	jp	__xcc_L3
__xcc_L1:
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	hl, (_array)
	ld	e, -18(ix)
	ld	d, -17(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	l, -20(ix)
	ld	h, -19(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	ld	l, -4(ix)
	ld	h, -3(ix)
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_92777
	ld	hl, #0
	jp	__cmp_e_36915
__cmp_t_92777:
	ld	hl, #1
__cmp_e_36915:
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	l, -24(ix)
	ld	h, -23(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L4
	jp	__xcc_L6
__xcc_L4:
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	.globl _swap
	call	_swap
	pop	bc
	pop	bc
	ld	l, -6(ix)
	ld	h, -5(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	ld	-6(ix), l
	ld	-5(ix), h
	jp	__xcc_L6
__xcc_L6:
__xcc_L2:
	ld	l, -8(ix)
	ld	h, -7(ix)
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	inc	hl
	ld	-8(ix), l
	ld	-7(ix), h
	jp	__xcc_L0
__xcc_L3:
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	.globl _swap
	call	_swap
	pop	bc
	pop	bc
	ld	l, -6(ix)
	ld	h, -5(ix)
	jp	__partition_end
__partition_end:
	; epilogue: partition
	ld	sp, ix
	pop	ix
	ret
	.globl _quicksort
_quicksort:
	; prologue: quicksort (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
	add	hl, sp
	ld	sp, hl
	; receive param left at 4(ix)
	; receive param right at 6(ix)
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	ld	l, 6(ix)
	ld	h, 7(ix)
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	p, __cmp_t_47793
	ld	hl, #0
	jp	__cmp_e_38335
__cmp_t_47793:
	ld	hl, #1
__cmp_e_38335:
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L7
	jp	__xcc_L9
__xcc_L7:
	jp	__quicksort_end
	jp	__xcc_L9
__xcc_L9:
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	.globl _partition
	call	_partition
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	hl
	dec	sp
	dec	sp
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -8(ix)
	ld	h, -7(ix)
	push	hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	push	hl
	.globl _quicksort
	call	_quicksort
	pop	bc
	pop	bc
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	.globl _quicksort
	call	_quicksort
	pop	bc
	pop	bc
__quicksort_end:
	; epilogue: quicksort
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=2)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-2
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
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, (_array)
	ld	e, -4(ix)
	ld	d, -3(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-6(ix), l
	ld	-5(ix), h
	ld	l, -6(ix)
	ld	h, -5(ix)
	push	hl
	ld	de, #62
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
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
	ld	-8(ix), l
	ld	-7(ix), h
	ld	hl, (_array)
	ld	e, -8(ix)
	ld	d, -7(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	push	hl
	ld	de, #83
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #2
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-12(ix), l
	ld	-11(ix), h
	ld	hl, (_array)
	ld	e, -12(ix)
	ld	d, -11(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-14(ix), l
	ld	-13(ix), h
	ld	l, -14(ix)
	ld	h, -13(ix)
	push	hl
	ld	de, #4
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #3
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-16(ix), l
	ld	-15(ix), h
	ld	hl, (_array)
	ld	e, -16(ix)
	ld	d, -15(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-18(ix), l
	ld	-17(ix), h
	ld	l, -18(ix)
	ld	h, -17(ix)
	push	hl
	ld	de, #89
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #4
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-20(ix), l
	ld	-19(ix), h
	ld	hl, (_array)
	ld	e, -20(ix)
	ld	d, -19(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-22(ix), l
	ld	-21(ix), h
	ld	l, -22(ix)
	ld	h, -21(ix)
	push	hl
	ld	de, #36
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #5
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-24(ix), l
	ld	-23(ix), h
	ld	hl, (_array)
	ld	e, -24(ix)
	ld	d, -23(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-26(ix), l
	ld	-25(ix), h
	ld	l, -26(ix)
	ld	h, -25(ix)
	push	hl
	ld	de, #21
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #6
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-28(ix), l
	ld	-27(ix), h
	ld	hl, (_array)
	ld	e, -28(ix)
	ld	d, -27(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-30(ix), l
	ld	-29(ix), h
	ld	l, -30(ix)
	ld	h, -29(ix)
	push	hl
	ld	de, #74
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #7
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-32(ix), l
	ld	-31(ix), h
	ld	hl, (_array)
	ld	e, -32(ix)
	ld	d, -31(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-34(ix), l
	ld	-33(ix), h
	ld	l, -34(ix)
	ld	h, -33(ix)
	push	hl
	ld	de, #37
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #8
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-36(ix), l
	ld	-35(ix), h
	ld	hl, (_array)
	ld	e, -36(ix)
	ld	d, -35(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-38(ix), l
	ld	-37(ix), h
	ld	l, -38(ix)
	ld	h, -37(ix)
	push	hl
	ld	de, #65
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #9
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-40(ix), l
	ld	-39(ix), h
	ld	hl, (_array)
	ld	e, -40(ix)
	ld	d, -39(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-42(ix), l
	ld	-41(ix), h
	ld	l, -42(ix)
	ld	h, -41(ix)
	push	hl
	ld	de, #33
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #10
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-44(ix), l
	ld	-43(ix), h
	ld	hl, (_array)
	ld	e, -44(ix)
	ld	d, -43(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-46(ix), l
	ld	-45(ix), h
	ld	l, -46(ix)
	ld	h, -45(ix)
	push	hl
	ld	de, #96
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #11
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-48(ix), l
	ld	-47(ix), h
	ld	hl, (_array)
	ld	e, -48(ix)
	ld	d, -47(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-50(ix), l
	ld	-49(ix), h
	ld	l, -50(ix)
	ld	h, -49(ix)
	push	hl
	ld	de, #38
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #12
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-52(ix), l
	ld	-51(ix), h
	ld	hl, (_array)
	ld	e, -52(ix)
	ld	d, -51(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-54(ix), l
	ld	-53(ix), h
	ld	l, -54(ix)
	ld	h, -53(ix)
	push	hl
	ld	de, #53
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #13
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-56(ix), l
	ld	-55(ix), h
	ld	hl, (_array)
	ld	e, -56(ix)
	ld	d, -55(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-58(ix), l
	ld	-57(ix), h
	ld	l, -58(ix)
	ld	h, -57(ix)
	push	hl
	ld	de, #16
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #14
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-60(ix), l
	ld	-59(ix), h
	ld	hl, (_array)
	ld	e, -60(ix)
	ld	d, -59(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-62(ix), l
	ld	-61(ix), h
	ld	l, -62(ix)
	ld	h, -61(ix)
	push	hl
	ld	de, #74
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	hl, #15
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-64(ix), l
	ld	-63(ix), h
	ld	hl, (_array)
	ld	e, -64(ix)
	ld	d, -63(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-66(ix), l
	ld	-65(ix), h
	ld	l, -66(ix)
	ld	h, -65(ix)
	push	hl
	ld	de, #55
	pop	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L10:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #16
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
	ld	-68(ix), l
	ld	-67(ix), h
	ld	l, -68(ix)
	ld	h, -67(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L11
	jp	__xcc_L13
__xcc_L11:
	ld	hl, #__str_14
	dec	sp
	dec	sp
	ld	-70(ix), l
	ld	-69(ix), h
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
	ld	-72(ix), l
	ld	-71(ix), h
	ld	hl, (_array)
	ld	e, -72(ix)
	ld	d, -71(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-74(ix), l
	ld	-73(ix), h
	ld	l, -74(ix)
	ld	h, -73(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-76(ix), l
	ld	-75(ix), h
	ld	l, -76(ix)
	ld	h, -75(ix)
	push	hl
	ld	l, -70(ix)
	ld	h, -69(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-78(ix), l
	ld	-77(ix), h
__xcc_L12:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-80(ix), l
	ld	-79(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L10
__xcc_L13:
	ld	hl, #__str_15
	dec	sp
	dec	sp
	ld	-82(ix), l
	ld	-81(ix), h
	ld	l, -82(ix)
	ld	h, -81(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-84(ix), l
	ld	-83(ix), h
	ld	hl, #15
	push	hl
	ld	hl, #0
	push	hl
	.globl _quicksort
	call	_quicksort
	pop	bc
	pop	bc
	ld	hl, #0
	ld	-2(ix), l
	ld	-1(ix), h
__xcc_L16:
	ld	l, -2(ix)
	ld	h, -1(ix)
	push	hl
	ld	hl, #16
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, __cmp_t_16649
	ld	hl, #0
	jp	__cmp_e_41421
__cmp_t_16649:
	ld	hl, #1
__cmp_e_41421:
	dec	sp
	dec	sp
	ld	-86(ix), l
	ld	-85(ix), h
	ld	l, -86(ix)
	ld	h, -85(ix)
	ld	a, h
	or	a, l
	jp	nz, __xcc_L17
	jp	__xcc_L19
__xcc_L17:
	ld	hl, #__str_20
	dec	sp
	dec	sp
	ld	-88(ix), l
	ld	-87(ix), h
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
	ld	-90(ix), l
	ld	-89(ix), h
	ld	hl, (_array)
	ld	e, -90(ix)
	ld	d, -89(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-92(ix), l
	ld	-91(ix), h
	ld	l, -92(ix)
	ld	h, -91(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
	dec	sp
	dec	sp
	ld	-94(ix), l
	ld	-93(ix), h
	ld	l, -94(ix)
	ld	h, -93(ix)
	push	hl
	ld	l, -88(ix)
	ld	h, -87(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-96(ix), l
	ld	-95(ix), h
__xcc_L18:
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	sp
	dec	sp
	ld	-98(ix), l
	ld	-97(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	inc	hl
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__xcc_L16
__xcc_L19:
	ld	hl, #__str_21
	dec	sp
	dec	sp
	ld	-100(ix), l
	ld	-99(ix), h
	ld	l, -100(ix)
	ld	h, -99(ix)
	push	hl
	.globl _printf
	call	_printf
	pop	bc
	dec	sp
	dec	sp
	ld	-102(ix), l
	ld	-101(ix), h
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
