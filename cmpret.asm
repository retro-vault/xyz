;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.20 #16281 (Linux)
;--------------------------------------------------------
	.module cmpret
	
	.optsdcc -mz80 sdcccall(1)
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _nz8
	.globl _lt16
	.globl _eq16
	.globl _eq8
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
;--------------------------------------------------------
; absolute ram data
;--------------------------------------------------------
	.area _DABS (ABS)
	.area _DABS (ABS)
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area _HOME
	.area _GSINIT
	.area _GSFINAL
	.area _GSINIT
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area _HOME
	.area _HOME
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area _CODE
;/tmp/tmp.pe1expToNi/cmpret.c:1: unsigned char eq8(unsigned char a, unsigned char b) { return a == b; }
;	---------------------------------
; Function eq8
; ---------------------------------
_eq8::
	sub	a, l
	ld	a, #0x01
	ret	Z
	xor	a, a
	ret
;/tmp/tmp.pe1expToNi/cmpret.c:2: unsigned int eq16(unsigned int a, unsigned int b) { return a == b; }
;	---------------------------------
; Function eq16
; ---------------------------------
_eq16::
	cp	a, a
	sbc	hl, de
	ld	a, #0x01
	jp	Z, 00104$
	xor	a, a
00104$:
	ld	e, a
	ld	d, #0x00
	ret
;/tmp/tmp.pe1expToNi/cmpret.c:3: unsigned int lt16(unsigned int a, unsigned int b) { return a < b; }
;	---------------------------------
; Function lt16
; ---------------------------------
_lt16::
	xor	a, a
	sbc	hl, de
	ld	a, #0x00
	rla
	ld	e, a
	ld	d, #0x00
	ret
;/tmp/tmp.pe1expToNi/cmpret.c:4: unsigned char nz8(unsigned char a) { return a != 0; }
;	---------------------------------
; Function nz8
; ---------------------------------
_nz8::
	sub	a, #0x01
	ld	a, #0x00
	rla
	xor	a, #0x01
	ret
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
