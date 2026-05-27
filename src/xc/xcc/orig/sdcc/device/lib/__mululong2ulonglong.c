/*-------------------------------------------------------------------------
   __mululong2ulonglong.c - routine for unsigned 32x32->64 multiplication

   Copyright (C) 2026, Philipp Klaus Krause . philipp@colecovision.eu

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

#include <sdcc-lib.h>

#if defined(__SDCC_r4k) || defined(__SDCC_r5k) || defined(__SDCC_r6k) || defined(__SDCC_r800)
// Version for targets that have hardware support for unsigned 16x16->32 multiplication.
uint64_t __mululong2ulonglong(uint32_t l, uint32_t r) __SDCC_NONBANKED
{
  uint16_t l0 = l;
  uint16_t l1 = l >> 16;
  uint16_t r0 = r;
  uint16_t r1 = r >> 16;

  uint32_t m00 = (uint32_t)l0 * r0;
  uint32_t m10 = (uint32_t)l1 * r0;
  uint32_t m01 = (uint32_t)l0 * r1;
  uint32_t m11 = (uint32_t)l1 * r1;

  return (((uint64_t)m11 << 32) + ((uint64_t)m10 << 16) + ((uint64_t)m01 << 16) + m00);
}
#else
// Version using 8x8->16 multiplications.
unsigned long long __mululong2ulonglong(unsigned long ll, unsigned long lr) __SDCC_NONBANKED
{
  unsigned long long ret = 0ull;
  unsigned char i, j;

  for (i = 0; i < sizeof (long); i++)
    {
      unsigned char l = ll >> (i * 8);
      for(j = 0; j < sizeof (long); j++)
        {
          unsigned char r = lr >> (j * 8);
          ret += (unsigned long long)((unsigned short)(l * r)) << ((i + j) * 8);
        }
    }

  return(ret);
}
#endif

