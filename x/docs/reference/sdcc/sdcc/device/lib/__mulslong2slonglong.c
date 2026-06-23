/*-------------------------------------------------------------------------
   __mulslong2slonglong.c - routine for signed 32x32->64 multiplication

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
#include <stdbool.h>

#include <sdcc-lib.h>

int64_t __mulslong2slonglong(int32_t l, int32_t r) __SDCC_NONBANKED
{
  bool negative = false;
  if (l < 0)
    {
      l = -l;
      negative = !negative;
    }
  if (r < 0)
    {
      r = -r;
      negative = !negative;
    }
  int64_t ret = (uint64_t)(uint32_t)l * (uint32_t)r;
  return(negative ? -ret : ret);
}

