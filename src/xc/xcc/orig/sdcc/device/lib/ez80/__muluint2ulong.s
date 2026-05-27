;--------------------------------------------------------------------------
;  __muluint2ulong.s
;
;  Copyright (c) 2026, Philipp Klaus Krause
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

.module __muluint2ulong

.ez80
.optsdcc -mez80 sdcccall(1)
  
.globl ___muluint2ulong

; uint32_t __muluint2ulong (uint16_t l, uint16_t r);

.area _CODE

___muluint2ulong:

ld	iy, #0
add	iy, sp

ld	a, l

ld	d, l
ld	e, 2(iy)
ld	l, e
mlt	de
ld	c, d
ld	b, #0

ld	d, h

mlt	hl
add	hl, bc

ld	c, a
ld	b, 3(iy)
ld	a, b
mlt	bc
add	hl, bc

ld	c, d

ld	d, l
ld	l, h
ld	h, #0
rl	h

ld	b, a
mlt	bc
add	hl, bc

ret

