;-------------------------------------------------------------------------
;   rand.s - standard C library function
;
;   Copyright (C) 2026, Gabriele Gorla
;
;   This library is free software; you can redistribute it and/or modify it
;   under the terms of the GNU General Public License as published by the
;   Free Software Foundation; either version 2, or (at your option) any
;   later version.
;
;   This library is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this library; see the file COPYING. If not, write to the
;   Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
;   MA 02110-1301, USA.
;
;   As a special exception, if you link this library with other files,
;   some of which are compiled with SDCC, to produce an executable,
;   this library does not by itself cause the resulting executable to
;   be covered by the GNU General Public License. This exception does
;   not however invalidate any other reasons why the executable file
;   might be covered by the GNU General Public License.
;-------------------------------------------------------------------------
	.module rand
	
	.optsdcc -mmos6502

;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _rand
	.globl _srand

;--------------------------------------------------------
; overlayable items in ram
;--------------------------------------------------------
	.area	OSEG    (PAG, OVR)
_rand_sloc0_1_0:
	.ds 4
_rand_sloc1_1_0:
	.ds 4

;--------------------------------------------------------
; uninitialized external ram data
;--------------------------------------------------------
	.area BSS
;--------------------------------------------------------
; initialized external ram data
;--------------------------------------------------------
	.area DATA
_seed:
	.ds 4

;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area CODE
;------------------------------------------------------------
;sloc0         Allocated with name '_rand_sloc0_1_0'
;sloc1         Allocated with name '_rand_sloc1_1_0'
;------------------------------------------------------------
_rand:
;	../rand.c: 44: t ^= t >> 10;
	lda	(_seed+3) 	; A:--:???  X:--:???  Y:--:???  F:a C:?
;	sta	*(_rand_sloc0_1_0 + 3) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	lsr	a 	; A:U-:???  X:--:???  Y:--:???  F:a C:?
	sta	*(_rand_sloc0_1_0 + 2) 	; A:U-:???  X:--:???  Y:--:???  F:a C:?
	lda	(_seed+2) 	; A:U-:???  X:--:???  Y:--:???  F:a C:?
	ror	a 	; A:U-:???  X:--:???  Y:--:???  F:a C:?
	sta	*(_rand_sloc0_1_0 + 1) 	; A:U-:???  X:--:???  Y:--:???  F:a C:?
	lda	(_seed+1) 	; A:U-:???  X:--:???  Y:--:???  F:a C:?
	ror	a 	; A:U-:???  X:--:???  Y:--:???  F:a C:?
	sta	*_rand_sloc0_1_0 	; A:U-:???  X:--:???  Y:--:???  F:a C:?

	lda	*(_rand_sloc0_1_0 + 2) 	; A:U-:???  X:--:???  Y:--:???  F:a C:?
	lsr	a 	; A:U-:???  X:--:???  Y:--:???  F:a C:?
	eor	(_seed+2) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
	sta	*(_rand_sloc0_1_0 + 2) 	; A:U-:???  X:--:???  Y:--:???  F:? C:?
	lda	*(_rand_sloc0_1_0 + 1) 	; A:U-:???  X:--:???  Y:--:???  F:? C:?
	ror	a
	eor	(_seed+1) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
	sta	*(_rand_sloc0_1_0 + 1) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	lda	*_rand_sloc0_1_0 	; A:U-:???  X:--:???  Y:--:???  F:? C:?
	ror 	a
	eor	_seed 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
	sta	*_rand_sloc0_1_0 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?

;	lda	(_seed+1) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
;	eor	*(_rand_sloc0_1_0 + 1) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
;	sta	*(_rand_sloc0_1_0 + 1) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
;	lda	(_seed+2) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
;	eor	*(_rand_sloc0_1_0 + 2) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
;	sta	*(_rand_sloc0_1_0 + 2) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?

;	../rand.c: 45: t ^= t << 9;
	lda	*_rand_sloc0_1_0 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
	asl	a 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	sta	*(_rand_sloc1_1_0 + 1) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	lda	*(_rand_sloc0_1_0 + 1) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	rol	a 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	sta	*(_rand_sloc1_1_0 + 2) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	lda	*(_rand_sloc0_1_0 + 2) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	rol	a 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
;	sty	*_rand_sloc1_1_0 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
;	sta	*(_rand_sloc1_1_0 + 3) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?

;	lda	*(_rand_sloc0_1_0 + 3) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
;	eor	*(_rand_sloc0_1_0 + 3) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
;	sta	*(_rand_sloc0_1_0 + 3) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	tax
	lda	*(_rand_sloc0_1_0 + 2) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
	eor	*(_rand_sloc1_1_0 + 2) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	sta	*(_rand_sloc0_1_0 + 2) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	lda	*(_rand_sloc0_1_0 + 1) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
	eor	*(_rand_sloc1_1_0 + 1) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	sta	*(_rand_sloc0_1_0 + 1) 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	txa
;	lda	*(_rand_sloc0_1_0 + 3) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?

;	../rand.c: 46: t ^= t >> 25;
	lsr	a 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	eor	*_rand_sloc0_1_0 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
;	sta	*_rand_sloc0_1_0 	; A:U-:???  X:--:???  Y:--:#00  F:a C:?
	tay

;	../rand.c: 48: s = t; return(t & RAND_MAX);
;	lda	*(_rand_sloc0_1_0 + 3) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
	stx	(_seed+3) 	; A:U-:A+3  X:--:???  Y:--:#00  F:a C:?
	lda	*(_rand_sloc0_1_0 + 2) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
	sta	(_seed+2) 	; A:U-:A+2  X:--:???  Y:--:#00  F:a C:?
	lda	*(_rand_sloc0_1_0 + 1) 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
	sta	(_seed+1) 	; A:U-:A+1  X:--:???  Y:--:#00  F:a C:?
	and	#0x7f 	; A:U-:???  X:U-:A+1  Y:U-:A+0  F:a C:?
	tax
;	lda	*_rand_sloc0_1_0 	; A:--:???  X:--:???  Y:--:#00  F:a C:?
	tya
	sta	_seed 	; A:U-:A+0  X:--:???  Y:--:#00  F:a C:?
	rts	 	; A:--:???  X:--:???  Y:--:???  F:? C:?

_srand:
	sta	_seed
	stx	(_seed+1)
	lda	#0x00
	sta	(_seed+2)
	lda	#0x80
	sta	(_seed+3)
	rts

	.area XINIT
__xinit__seed:
	.byte #0x01, #0x00, #0x00, #0x80	; 2147483649
