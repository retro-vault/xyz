/*-------------------------------------------------------------------------
  tempops.c - source file for regtemp operations for the MOS6502

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

static reg_info *save_reg = NULL;

bool
m6502_fastSaveAi(reg_info *reg)
{
  if(reg->isFree && reg->isDead)
    {
      m6502_transferRegReg(m6502_reg_a, reg, true);
      save_reg = reg;
    }
  else
    {
      storeRegTemp(m6502_reg_a, true);
      save_reg = NULL;
    }
  return true;
}

bool
m6502_fastSaveA()
{
  return m6502_fastSaveAi(m6502_reg_y);
}

bool
m6502_fastRestoreA()
{
  if(save_reg)
    m6502_transferRegReg(save_reg, m6502_reg_a, true);
  else
    m6502_loadRegTemp(m6502_reg_a);

  return true;
}

/**************************************************************************
 * Store register onto the REGTEMP stack. If freereg is true,
 * reg is marked free and available for reuse. If force is true,
 * no literal optimizations are performed.
 *
 * @param reg pointer for the register to save
 * @param freereg free the register if true
 *************************************************************************/
bool
m6502_storeRegTempi(reg_info * reg, bool freereg, bool force)
{
  m6502_emitComment (REGOPS, "  storeRegTemp(%s) %s", reg ? reg->name : "-", freereg ? "free" : "");

  int regidx = reg->rIdx;
  char storeOp[4] = "st?";

  switch (regidx)
    {
    case A_IDX:
    case X_IDX:
    case Y_IDX:
      storeOp[2]=reg->name[0];
      _S.tempAttr[_S.tempOfs].isLiteral=reg->isLitConst;
      _S.tempAttr[_S.tempOfs].literalValue=reg->litConst;
      _S.tempAttr[_S.tempOfs].aop=reg->aop;
      _S.tempAttr[_S.tempOfs].aopofs=reg->aopofs;
      _S.tempAttr[_S.tempOfs].stackOffset=reg->stackOffset;
      if(reg->isLitConst && !force)
        {
	  m6502_emitComment(REGOPS|VVDBG, "  %s: virtual store literal 0x%02x",__func__,
			    (unsigned char)reg->litConst);
        }
      else if(reg->aop && !force && (reg->aop->type==AOP_DIR || reg->aop->type==AOP_EXT) )
        {
          m6502_emitComment(REGOPS|VVDBG, "  %s: virtual store %s+%d",__func__,
			    reg->aop->aopu.aop_dir, reg->aopofs);
        }
      else 
        {    
	  m6502_emitOp (storeOp, TEMPFMT, _S.tempOfs);
        }
      _S.tempOfs++;
      break;
    case XA_IDX:
      m6502_storeRegTempi (m6502_reg_a, freereg, force);
      m6502_storeRegTempi (m6502_reg_x, freereg, force);
      break;
    case XY_IDX:
      m6502_storeRegTempi (m6502_reg_y, freereg, force);
      m6502_storeRegTempi (m6502_reg_x, freereg, force);
      break;
    default:
      emitcode("ERROR", "%s : bad reg %02x (%s)", __func__, regidx, reg->name);
      break;
    }
  
  if (freereg)
    m6502_freeReg (reg);

  if(_S.tempOfs > NUM_TEMP_REGS)
    emitcode("ERROR", "storeRegTemp(): overflow");

  return true;
}

/**************************************************************************
 * Load register from the REGTEMP stack at an arbitrary offset
 *
 * @param reg pointer for the register to save
 *************************************************************************/
void
m6502_loadRegTempAt (reg_info * reg, int offset)
{
  char loadOp[4] = "ld?";
  
  if (offset<0 || offset>_S.tempOfs)
    {
      emitcode("ERROR", " %s - called with illegal offset %d (tempOfs=%d)", __func__, offset, _S.tempOfs);
      return;
    }

  if(_S.tempAttr[offset].isLiteral)
    {
      m6502_loadRegFromConst(reg, _S.tempAttr[offset].literalValue);
      return;
    }

  m6502_dirtyReg (reg);

  switch (reg->rIdx)
    {
    case A_IDX:
    case X_IDX:
    case Y_IDX:
      loadOp[2]=reg->name[0];
      if(_S.tempAttr[offset].aop && (_S.tempAttr[offset].aop->type==AOP_DIR || _S.tempAttr[offset].aop->type==AOP_EXT))
        {
          m6502_emitComment(REGOPS|VVDBG, "  %s: should load from %s+%d", __func__,
			    _S.tempAttr[offset].aop->aopu.aop_dir, _S.tempAttr[offset].aopofs);
          if(_S.tempAttr[offset].aop->type==AOP_DIR)
	    m6502_emitOp (loadOp, "*(%s+%d)",
			  _S.tempAttr[offset].aop->aopu.aop_dir, _S.tempAttr[offset].aopofs );
          else
	    m6502_emitOp (loadOp, "(%s+%d)",
			  _S.tempAttr[offset].aop->aopu.aop_dir, _S.tempAttr[offset].aopofs );
        }
      else
        {
          m6502_emitOp (loadOp, TEMPFMT, offset);
        }
      break;
    default:
      emitcode("ERROR","%s - called with illegal regidx %d", __func__, reg->rIdx);
    }
  
  reg->aop=_S.tempAttr[offset].aop;
  reg->aopofs=_S.tempAttr[offset].aopofs;
  reg->stackOffset=_S.tempAttr[offset].stackOffset;

  m6502_useReg (reg);
}

/**************************************************************************
 * Load register from the REGTEMP stack.
 *
 * @param reg pointer for the register to save
 *************************************************************************/
void
m6502_loadRegTemp (reg_info * reg)
{

  if(_S.tempOfs==0)
    {
      emitcode("ERROR", "%s - temp stack is empty", __func__);
      return;
    }

  // pop off stack, unused
  if (reg == NULL)
    {
      _S.tempOfs--;
      return;
    }

  switch (reg->rIdx)
    {
    case A_IDX:
    case X_IDX:
    case Y_IDX:
      m6502_loadRegTempAt(reg, --_S.tempOfs);
      return;
    case XA_IDX:
      m6502_loadRegTemp(m6502_reg_x);
      m6502_loadRegTemp(m6502_reg_a);
      break;
    case XY_IDX:
      m6502_loadRegTemp(m6502_reg_x);
      m6502_loadRegTemp(m6502_reg_y);
      break;
    default:
      emitcode("ERROR", "%s - called with illegal regidx %d", __func__, reg->rIdx);
      break;
    }

  // FIXME: figure out if register pairs are literals
  m6502_useReg (reg);
  m6502_dirtyReg (reg);
}

/**************************************************************************
 * Conditionally load a register from the REGTEMP stack.
 *
 * @param reg pointer for the register to load
 * @param load register if true otherwise free the register
 *************************************************************************/
void
m6502_loadOrFreeRegTemp (reg_info * reg, bool needpull)
{
  if (needpull)
    m6502_loadRegTemp (reg);
  else
    m6502_freeReg (reg);
}

/**************************************************************************
 * Conditionally load a register from the REGTEMP stack without
 * affecting condition flags
 *
 * @param reg pointer for the register to load
 * @param load register if true otherwise free the register
 *************************************************************************/
void
m6502_loadRegTempNoFlags (reg_info * reg, bool needpull)
{
  if (needpull)
    {
      int tempflag=_S.lastflag;
      m6502_emitOp("php", "");
      m6502_loadRegTemp (reg);
      m6502_emitOp("plp", "");
      _S.lastflag=tempflag;
    }
  else
    {
      m6502_freeReg (reg);
    }
}

void
m6502_emitRegTempOp(const char *op, int offset)
{

  if (offset<0 || offset>=_S.tempOfs)
    {
      emitcode("ERROR", " %s - called with illegal offset %d (tempOfs=%d)", __func__, offset, _S.tempOfs);
      return;
    }

  if(!strcmp(op,"asl") || !strcmp(op,"lsr") || !strcmp(op,"rol") || !strcmp(op,"ror") )
    m6502_dirtyRegTemp (offset);

  // prevent const optimization for "bit" on the plain 6502
  if(!strcmp(op,"bit") && _S.tempAttr[offset].isLiteral && !IS_MOS65C02)
    m6502_dirtyRegTemp (offset);

  if(_S.tempAttr[offset].isLiteral)
    {
      m6502_emitOp(op, IMMDFMT, _S.tempAttr[offset].literalValue );
    }
  else if(_S.tempAttr[offset].aop && (_S.tempAttr[offset].aop->type==AOP_DIR || _S.tempAttr[offset].aop->type==AOP_EXT))
    {
      m6502_emitComment(REGOPS|VVDBG, "  %s: %s with %s+%d", __func__,
			op, _S.tempAttr[offset].aop->aopu.aop_dir, _S.tempAttr[offset].aopofs);
      m6502_emitOp (op, "%s(%s+%d)", (_S.tempAttr[offset].aop->type==AOP_DIR)?"*":"",
		    _S.tempAttr[offset].aop->aopu.aop_dir, _S.tempAttr[offset].aopofs );
    }
  else
    {
      m6502_emitOp(op, TEMPFMT, offset);
      //      if(!strcmp(op,"asl") || !strcmp(op,"lsr") || !strcmp(op,"rol") || !strcmp(op,"ror")
      //         || !strcmp(op,"inc") || !strcmp(op,"dec") )
      //      m6502_dirtyRegTemp (offset);
    }
}

void
m6502_dirtyRegTemp (int offset)
{
  _S.tempAttr[offset].isLiteral=false;
  _S.tempAttr[offset].aop=NULL;
}

