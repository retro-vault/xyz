;-------------------------------------------------------------------------
;   labs.s - standard C library function
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

	.module labs

;--------------------------------------------------------
; exported symbols
;--------------------------------------------------------
	.globl _labs
	.globl _labs_PARM_1
	
;--------------------------------------------------------
; overlayable function parameters in zero page
;--------------------------------------------------------
	.area	OSEG    (PAG, OVR)
_labs_PARM_1:
	.ds 4	
	
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area CODE

_labs:
	lda	*(_labs_PARM_1+3)
	bpl	copy
  	sec
	lda	#0x00
	sbc	*(_labs_PARM_1+0)
	tay
	lda	#0x00
	sbc	*(_labs_PARM_1+1)
	tax
	lda	#0x00
	sbc	*(_labs_PARM_1+2)
	sta	*___SDCC_m6502_ret2
	lda	#0x00
	sbc	*(_labs_PARM_1+3)
	sta	*___SDCC_m6502_ret3
	tya
	rts
copy:
	sta	*___SDCC_m6502_ret3
	lda	*(_labs_PARM_1+2)
	sta	*___SDCC_m6502_ret2
	ldx	*(_labs_PARM_1+1)
	lda	*(_labs_PARM_1+0)	
	rts

