;-------------------------------------------------------------------------
;   __stdc_bit_width.s - return the size in bits
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

	.module __stdc_bit_width

;--------------------------------------------------------
; exported symbols
;--------------------------------------------------------
	.globl ___stdc_bit_width_PARM_1
	.globl ___stdc_bit_width
	
;--------------------------------------------------------
; overlayable function parameters in zero page
;--------------------------------------------------------
	.area	OSEG    (PAG, OVR)
___stdc_bit_width_PARM_1:
	.ds 8

;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area CODE

___stdc_bit_width:
	ldx #0x07
	ldy #0x40

bloop:
	lda *___stdc_bit_width_PARM_1,x
	asl a
	bcs end
    dey
	asl a
	bcs end
    dey
	asl a
	bcs end
    dey
	asl a
	bcs end
    dey
	asl a
	bcs end
    dey
	asl a
	bcs end
    dey
	asl a
	bcs end
    dey
	asl a
	bcs end
    dey

    dex
    bpl bloop

end:
	tya
	rts
