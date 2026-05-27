/*-------------------------------------------------------------------------
   stdc_memreverse8u.c - part of bit and byte utilitites

   Copyright (c) 2025, Philipp Klaus Krause . philipp@colecovision.eu

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING. If not, write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.

   As a special exception, if you link this library with other files,
   some of which are compiled with SDCC, to produce an executable,
   this library does not by itself cause the resulting executable to
   be covered by the GNU General Public License. This exception does
   not however invalidate any other reasons why the executable file
   might be covered by the GNU General Public License.
-------------------------------------------------------------------------*/

#include <stdint.h>

extern uint8_t stdc_memreverse8u8 (uint8_t value);
extern uint16_t stdc_memreverse8u16 (uint16_t value);
extern uint32_t stdc_memreverse8u32 (uint32_t value);
extern uint64_t stdc_memreverse8u64 (uint64_t value);

#include <stdbit.h>

