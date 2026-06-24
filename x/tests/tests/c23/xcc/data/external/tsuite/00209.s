	.module xcc_output

	.area _DATA
	.globl _e
_e:
	.ds 2
	.globl _e1
_e1:
	.ds 2
	.globl _e2
_e2:
	.ds 2
	.globl _s
_s:
	.ds 2
	.globl _s1
_s1:
	.ds 2
	.globl _s2
_s2:
	.ds 2


	.area _CODE

	.globl _f1
_f1:
	; prologue: f1 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param fp at 4(ix)
	; receive param i at 6(ix)
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
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
	.globl __call_hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	call	__call_hl
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	jp	__f1_end
__f1_end:
	; epilogue: f1
	ld	sp, ix
	pop	ix
	ret
	.globl _f2
_f2:
	; prologue: f2 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param fp at 4(ix)
	; receive param i at 6(ix)
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
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
	.globl __call_hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	call	__call_hl
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	jp	__f2_end
__f2_end:
	; epilogue: f2
	ld	sp, ix
	pop	ix
	ret
	.globl _f3
_f3:
	; prologue: f3 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param fp at 4(ix)
	; receive param i at 6(ix)
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
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
	.globl __call_hl
	ld	l, -2(ix)
	ld	h, -1(ix)
	call	__call_hl
	pop	bc
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	jp	__f3_end
__f3_end:
	; epilogue: f3
	ld	sp, ix
	pop	ix
	ret
	.globl _f4
_f4:
	; prologue: f4 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param fp at 4(ix)
	; receive param i at 12(ix)
	ld	l, 12(ix)
	ld	h, 13(ix)
	push	hl
	.globl __mul16
	ld	hl, #2
	push	hl
	ld	l, 12(ix)
	ld	h, 13(ix)
	push	hl
	call	__mul16
	pop	bc
	pop	bc
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, 4(ix)
	ld	h, 5(ix)
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	dec	sp
	dec	sp
	ld	-4(ix), l
	ld	-3(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ex	de, hl
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
	.globl __call_hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	call	__call_hl
	pop	bc
	dec	sp
	dec	sp
	ld	-10(ix), l
	ld	-9(ix), h
	ld	l, -10(ix)
	ld	h, -9(ix)
	jp	__f4_end
__f4_end:
	; epilogue: f4
	ld	sp, ix
	pop	ix
	ret
	.globl _f5
_f5:
	; prologue: f5 (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	; receive param fp at 4(ix)
	; receive param i at 6(ix)
	ld	l, 6(ix)
	ld	h, 7(ix)
	push	hl
	.globl __call_hl
	ld	l, 4(ix)
	ld	h, 5(ix)
	call	__call_hl
	pop	bc
	dec	sp
	dec	sp
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	jp	__f5_end
__f5_end:
	; epilogue: f5
	ld	sp, ix
	pop	ix
	ret
	.globl _main
_main:
	; prologue: main (locals=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #0
	jp	__main_end
__main_end:
	; epilogue: main
	ld	sp, ix
	pop	ix
	ret
