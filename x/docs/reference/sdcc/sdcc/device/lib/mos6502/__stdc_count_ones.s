;-------------------------------------------------------------------------
;   __stdc_count_ones.s - count ones in a long long
;
;   Copyright (C) 2025, Gabriele Gorla
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

	.module __stdc_count_ones

;--------------------------------------------------------
; exported symbols
;--------------------------------------------------------
	.globl ___stdc_count_ones_PARM_1
	.globl ___stdc_count_ones

;--------------------------------------------------------
; overlayable function parameters in zero page
;--------------------------------------------------------
	.area	OSEG    (PAG, OVR)
___stdc_count_ones_PARM_1:
	.ds 8

;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area CODE

___stdc_count_ones:
	ldx #0x07
	ldy #0x00
bloop:
	lda *___stdc_count_ones_PARM_1,x
	beq nextb
	lsr a
	bcc b7
	iny
b7:
	lsr a
	bcc b6
	iny
b6:
	lsr a
	bcc b5
	iny
b5:
	lsr a
	bcc b4
	iny
b4:
	lsr a
	bcc b3
	iny
b3:
	lsr a
	bcc b2
	iny
b2:
	lsr a
	bcc b1
	iny
b1:
	lsr a
	bcc b0
	iny
b0:

nextb:
    dex
    bpl bloop
    
	tya
	rts
