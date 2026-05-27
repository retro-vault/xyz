/*-------------------------------------------------------------------------
  stackops.c - source file for stack operations for the MOS6502

  Copyright (C) 1998, Sandeep Dutta . sandeep.dutta@usa.net
  Copyright (C) 1999, Jean-Louis VERN.jlvern@writeme.com
  Bug Fixes - Wojciech Stryjewski  wstryj1@tiger.lsu.edu (1999 v2.1.9a)
  Hacked for the HC08:
  Copyright (C) 2003, Erik Petrich
  Hacked for the MOS6502:
  Copyright (C) 2020, Steven Hugg  hugg@fasterlight.com
  Copyright (C) 2021-2025, Gabriele Gorla

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
  -------------------------------------------------------------------------*/

#include "m6502.h"
#include "ralloc.h"
#include "gen.h"
#include "dbuf_string.h"

/**************************************************************************
 * pushReg - Push register reg onto the stack. If freereg is true, reg is
 *           marked free and available for reuse.
 *************************************************************************/
bool
m6502_pushReg (reg_info * reg, bool freereg)
{
  bool needloada = false;

  m6502_emitComment (REGOPS, "  pushReg(%s) %s %s", reg->name, reg->isFree?"free":"", reg->isDead?"dead":"");

  switch (reg->rIdx)
    {
    case A_IDX:
      m6502_emitOp ("pha", "");
      m6502_updateCFA ();
      break;
    case X_IDX:
      if (IS_MOS65C02)
        {
          m6502_emitOp ("phx", "");
        }
      else
        {
          needloada = storeRegTempIfUsed (m6502_reg_a);
          m6502_transferRegReg (m6502_reg_x, m6502_reg_a, freereg);
          m6502_pushReg (m6502_reg_a, true);
          m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
        }
      m6502_updateCFA ();
      break;
    case Y_IDX:
      if (IS_MOS65C02)
        {
          m6502_emitOp ("phy", "");
        }
      else
        {
          needloada = storeRegTempIfUsed (m6502_reg_a);
          m6502_transferRegReg (m6502_reg_y, m6502_reg_a, freereg);
          m6502_pushReg (m6502_reg_a, true);
          m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
        }
      m6502_updateCFA ();
      break;
      // little-endian order
    case XA_IDX:
      if(!IS_MOS65C02)
        {
          if(m6502_reg_y->isFree)
            {
              m6502_transferRegReg (m6502_reg_a, m6502_reg_y, freereg);
              m6502_transferRegReg (m6502_reg_x, m6502_reg_a, freereg);
              m6502_pushReg(m6502_reg_a, true);
              m6502_transferRegReg (m6502_reg_y, m6502_reg_a, true);
              m6502_pushReg(m6502_reg_a, freereg);
            }
          else
            {
              storeRegTemp(m6502_reg_a, true);
              m6502_transferRegReg (m6502_reg_x, m6502_reg_a, freereg);
              m6502_pushReg(m6502_reg_a, true);
	      m6502_loadRegTemp(m6502_reg_a);
              m6502_pushReg(m6502_reg_a, freereg);
            }
        }
      else
        {
	  m6502_pushReg(m6502_reg_x, freereg);
	  m6502_pushReg(m6502_reg_a, freereg);
        }
      break;
    case XY_IDX:
      if(!IS_MOS65C02)
        {
          needloada = storeRegTempIfUsed (m6502_reg_a);
          m6502_pushReg(m6502_reg_x, freereg);
          m6502_pushReg(m6502_reg_y, freereg);
          m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
        }
      else
        {
          m6502_pushReg(m6502_reg_x, freereg);
          m6502_pushReg(m6502_reg_y, freereg);
        }
      break;
    default:
      emitcode("ERROR", "    %s: bad reg idx: 0x%02x", __func__, reg->rIdx);
      break;
    }
  if (freereg)
    m6502_freeReg (reg);
  
  return true;
}

/**************************************************************************
 * pullReg - Pull register reg off the stack.
 *************************************************************************/
void
m6502_pullReg (reg_info * reg)
{
  int regidx = reg->rIdx;

  m6502_emitComment (REGOPS, __func__ );

  switch (regidx) {
  case A_IDX:
    m6502_emitOp ("pla", "");
    m6502_updateCFA ();
    break;
  case X_IDX:
    if (IS_MOS65C02)
      {
        m6502_emitOp ("plx", "");
      }
    else
      {
        // FIXME: saving A makes regression fail
        //      bool needloada = storeRegTempIfUsed (m6502_reg_a);
        m6502_pullReg (m6502_reg_a);
        m6502_transferRegReg (m6502_reg_a, m6502_reg_x, true);
        //      m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
      }
    m6502_updateCFA ();
    break;
  case Y_IDX:
    if (IS_MOS65C02)
      {
        m6502_emitOp ("ply", "");
      }
    else
      {
        // FIXME: saving A makes regression fail
        //      bool needloada = storeRegTempIfUsed (m6502_reg_a);
        m6502_pullReg (m6502_reg_a);
        m6502_transferRegReg (m6502_reg_a, m6502_reg_y, true);
        //      m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
      }
    m6502_updateCFA ();
    break;
    // little-endian order
  case XA_IDX:
    m6502_pullReg(m6502_reg_a);
    m6502_pullReg(m6502_reg_x);
    break;
  case XY_IDX:
    m6502_pullReg(m6502_reg_y);
    m6502_pullReg(m6502_reg_x);
    break;
  default:
    emitcode("ERROR", "    %s: bad reg idx: 0x%02x", __func__, regidx);
    break;
  }
  m6502_useReg (reg);
  m6502_dirtyReg (reg);
}

/**************************************************************************
 * adjustStack - Adjust the stack pointer by n bytes.
 *************************************************************************/
// TODO: optimize for 65C02
void
m6502_adjustStack (int n)
{
  bool restore_a = false;
  bool restore_x = false;
  char *inst=NULL;

  int sa_cycle = (m6502_reg_a->isFree)?0:6;
  int sx_cycle = (m6502_reg_x->isFree)?0:6;
  int abs_n = (n>0)?n:-n;

  m6502_emitComment (TRACEGEN, "%s - adjust: %d", __func__, n );
  m6502_emitComment (REGOPS, "  %s reg:  %s", __func__, m6502_regInfoStr());

  // unrolled PHA      1b, 3c x n
  // unrolled PLA      1b, 4c x n + 4b, 6c if A is used
  // unrolled INX/DEX  1b, 2c x n + 4b, 4c + 4b, 6c if X is used
  // ADC               5b, 8c + 4b, 4c + 4b, 6c if A is used + 4b, 6c if X is used
  
  // TODO: implement optimize for space
  int stack = (n>0) ? 4*abs_n + sa_cycle : 3*abs_n ; // PLA : PHA
  int incdec = 2*abs_n + 4 + sx_cycle ; // INC : DEC
  int adc = 12 + sa_cycle + sx_cycle;

  if(m6502_reg_x->aop==&m6502_tsxaop)
    {
      adc-=2;
      incdec-=2;
    }

  m6502_emitComment (REGOPS|VVDBG, "  %s : cycles stk:%d  incdec:%d  adc:%d", __func__,
		     stack, incdec, adc);

  if(stack<=incdec && stack<=adc)
    {
      inst = (n>0) ? "pla" : "pha";
      if(n>0)
	restore_a=storeRegTempIfUsed (m6502_reg_a);

      while((abs_n--)>0)
        m6502_emitOp(inst, "");

      m6502_loadOrFreeRegTemp(m6502_reg_a, restore_a);
    }
  else
    {
      restore_x = storeRegTempIfUsed(m6502_reg_x);

      if(incdec<=adc)
        {
          inst = (n>0) ? "inx" : "dex";
          m6502_emitTSX();

          while((abs_n--)>0)
            m6502_emitOp(inst, "");
        }
      else
        {
          restore_a = storeRegTempIfUsed(m6502_reg_a);
          m6502_emitTSX();
          m6502_transferRegReg (m6502_reg_x, m6502_reg_a, true);
          m6502_emitSetCarry(0);
          m6502_emitOp ("adc", IMMDFMT, (unsigned int)(n & 0xff));
          m6502_transferRegReg (m6502_reg_a, m6502_reg_x, true);
        }

      _S.stackPushes -= n;
      m6502_emitOp ("txs", "");
      m6502_loadOrFreeRegTemp(m6502_reg_a, restore_a);
      m6502_loadOrFreeRegTemp(m6502_reg_x, restore_x);
    }
  m6502_updateCFA ();
}
