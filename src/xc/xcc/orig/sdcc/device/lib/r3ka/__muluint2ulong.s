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

.r3ka
.optsdcc -mr3ka sdcccall(1)
  
.globl ___muluint2ulong

; uint32_t __muluint2ulong (uint16_t l, uint16_t r);

.area _CODE

; An unusual aspect of the Rabbit 2000A to 3000A is that is has a signed
; 16x16 multiplication, but not unsigned one. So we use the former to
; implement the latter.

___muluint2ulong:

pop	iy

ex	de, hl
pop	hl
rl	h
rla
srl	h
push	hl
rl	d
rla
srl	d
ld	c, l
ld	b, h

mul
push	hl

bool	hl
ld	l, h
rrca
jr	nc, 1$
ld	hl, 2(sp)
1$:
rrca
jr	nc, 2$
add	hl, de
2$:
rr	hl

ld	e, c
ld	c, a
rra
and	a, #0x80
add	a, b
ld	d, a
ld	a, c
pop	bc
adc	hl, bc

ld	b, a
rra
and	a, b
and	a, #0x40
add	a, h
ld	h, a

jp	(iy)

