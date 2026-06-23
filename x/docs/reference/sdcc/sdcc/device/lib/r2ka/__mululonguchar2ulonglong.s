;--------------------------------------------------------------------------
;  __mululonguchar2ulonglong.s
;
;  Copyright (c) 2025, Philipp Klaus Krause
;
;  This library is free software; you can redistribute it and/or modify it
;  under the terms of the GNU General Public License as published by the
;  Free Software Foundation; either version 2, or (at your option) any
;  later version.
;
;  This library is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License 
;  along with this library; see the file COPYING. If not, write to the
;  Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
;   MA 02110-1301, USA.
;
;  As a special exception, if you link this library with other files,
;  some of which are compiled with SDCC, to produce an executable,
;  this library does not by itself cause the resulting executable to
;  be covered by the GNU General Public License. This exception does
;  not however invalidate any other reasons why the executable file
;   might be covered by the GNU General Public License.
;--------------------------------------------------------------------------

.globl ___mululonguchar2ulonglong

.area _CODE

___mululonguchar2ulonglong:
	ld	iy, 2(sp)

	push	hl
	push	de
	ld	c, e

	ld	d, #0x00
	ld	e, a

	ld	b, d
	mul
	ld	0 (iy), c
	ld	a, b

	ld	hl, 0 (sp)
	ld	c, h
	ld	b, d
	mul
	add	a, c
	ld	1 (iy), a
	ld	a, b

	ld	hl, 1(sp)
	ld	c, h
	ld	b, d
	mul
	adc	a, c
	ld	2 (iy), a
	ld	a, b

	ld	hl, 2 (sp)
	ld	c, h
	ld	b, d
	mul
	adc	a, c
	ld	3 (iy), a

	ld	a, d
	adc	a, b
	ld	l, a

	ld	4 (iy), hl
	ld	l, d
	ld	6 (iy), hl

	add	sp, #4

	ret

