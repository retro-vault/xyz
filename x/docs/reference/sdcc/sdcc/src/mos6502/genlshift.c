/*-------------------------------------------------------------------------
  genlshift.c - source file for left shift code generation for the MOS6502

  Copyright (C) 1998, Sandeep Dutta . sandeep.dutta@usa.net
  Copyright (C) 1999, Jean-Louis VERN.jlvern@writeme.com
  Bug Fixes - Wojciech Stryjewski  wstryj1@tiger.lsu.edu (1999 v2.1.9a)
  Hacked for the HC08:
  Copyright (C) 2003, Erik Petrich
  Hacked for the MOS6502:
  Copyright (C) 2020, Steven Hugg  hugg@fasterlight.com
  Copyright (C) 2021-2026, Gabriele Gorla

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
 * m6502_AccLsh - left shift accumulator by known count
 *************************************************************************/
void
m6502_AccLsh (int shCount)
{
  int i;

  shCount &= 0x0007;            // shCount : 0..7

  if(shCount==7)
    {
      m6502_emitOp ("ror", "a");
      m6502_loadRegFromConst(m6502_reg_a, 0);
      m6502_emitOp ("ror", "a");
    }
  else if(shCount==6)
    {
      m6502_emitOp ("ror", "a");
      m6502_emitOp ("ror", "a");
      m6502_emitOp ("ror", "a");
      m6502_emitOp ("and", "#0xc0");
    }
  else
    {
      /* asl a is 2 cycles and 1 byte, so an unrolled loop is the   */
      /* fastest and shortest (shCount<6).                          */
      for (i = 0; i < shCount; i++)
	m6502_emitOp ("asl", "a");
    }
}


/**************************************************************************
 * XAccLsh - left shift register pair XA by known count
 *************************************************************************/
void
XAccLsh (reg_info *msb_reg, int shCount)
{
  int i;

  shCount &= 0x000f;            // shCount : 0..15

  if (shCount >= 8)
    {
      m6502_AccLsh (shCount - 8);
      m6502_transferRegReg (m6502_reg_a, msb_reg, false);
      m6502_loadRegFromConst (m6502_reg_a, 0);
    }
  else if(shCount==7)
    {
      storeRegTempAlways(msb_reg, true);
      m6502_emitRegTempOp ("lsr", m6502_getLastTempOfs());
      m6502_dirtyRegTemp(m6502_getLastTempOfs() );
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_transferRegReg (m6502_reg_a, msb_reg, false);
      m6502_loadRegFromConst (m6502_reg_a, 0);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_loadRegTemp(NULL);
    }
  else if(shCount!=0)
    {
      /* lsla/rolx is only 2 cycles and bytes, so an unrolled loop is often  */
      /* the fastest and shortest.                                           */
      storeRegTempAlways(msb_reg, true);
      for (i = 0; i < shCount; i++)
	{
	  m6502_rmwWithReg ("asl", m6502_reg_a);
	  m6502_emitRegTempOp ("rol", m6502_getLastTempOfs());
	  m6502_dirtyRegTemp(m6502_getLastTempOfs() );
	}
      m6502_loadRegTemp(msb_reg);
    }
}

/**************************************************************************
 * genlsh8 - left shift a one byte quantity by known count
 *************************************************************************/
static void
genlsh8 (operand * result, operand * left, int shCount)
{
  sym_link *resulttype = operandType (result);
  unsigned bytemask = (IS_BITINT (resulttype) && SPEC_USIGN (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;
  bool maskedbyte = (bytemask != 0xff);
  bool needpulla = false;

  m6502_emitComment (TRACEGEN, "  %s - shift=%d", __func__, shCount);
  if (shCount==0)
    return;

  if (!IS_AOP_A(AOP(result)) && m6502_sameRegs (AOP (left), AOP (result)) 
      && shCount<3 && m6502_aopCanShift(AOP(left)) && !maskedbyte)
    {
      while (shCount--)
        m6502_rmwWithAop ("asl", AOP (result), 0);
    }
  else
    {
      if(!IS_AOP_A(AOP(result)))
	needpulla = pushRegIfSurv (m6502_reg_a);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_AccLsh (shCount);
      if (maskedbyte)
	m6502_emitOp ("and", IMMDFMT, bytemask);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      pullOrFreeReg (m6502_reg_a, needpulla);
    }
}

/**************************************************************************
 * genlsh16 - left shift two bytes by known amount != 0
 *************************************************************************/
static void
genlsh16 (operand * result, operand * left, int shCount)
{
  bool needpulla = false;
  bool needpullx = false;

  sym_link *resulttype = operandType (result);
  unsigned topbytemask = (IS_BITINT (resulttype) && SPEC_USIGN (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;
  bool maskedtopbyte = (topbytemask != 0xff);

  m6502_emitComment (TRACEGEN, "  %s - shift=%d", __func__, shCount);

  if (shCount >= 8)
    {
      shCount -= 8;
      // TODO
      needpulla = pushRegIfSurv (m6502_reg_a);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_AccLsh (shCount);
      if (maskedtopbyte)
	m6502_emitOp ("and", IMMDFMT, topbytemask);

      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      m6502_storeConstToAop (0, AOP (result), 0);
      pullOrFreeReg (m6502_reg_a, needpulla);
      goto done; // Top byte is 0, doesn't need masking.
    }
  else if(IS_AOP_XA(AOP(result)))
    {
      if(shCount==1 && !IS_AOP_XA(AOP(left)))
        {
          m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  m6502_emitOp ("asl", "a");
          m6502_dirtyReg(m6502_reg_a);
	  m6502_fastSaveA();
          m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  m6502_emitOp ("rol", "a");
          m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);
	  m6502_fastRestoreA();
        }
      else
        {
	  /*  1 <= shCount <= 7 */
	  // TODO: count > 2 efficient?
	  m6502_loadRegFromAop (m6502_reg_xa, AOP (left), 0);
	  XAccLsh (m6502_reg_x, shCount);
	}
    }
  else if(m6502_aopCanShift(AOP(result)) && shCount <= 4)
    {
      if( m6502_sameRegs (AOP (left), AOP (result)))
	{
	  while (shCount--)
	    {
	      m6502_rmwWithAop ("asl", AOP (result), 0);
	      m6502_rmwWithAop ("rol", AOP (result), 1);
	    }
	}
      else
	{
	  needpulla = storeRegTempIfSurv (m6502_reg_a);
          if(IS_AOP_WITH_X(AOP(left)) && AOP_TYPE(result)==AOP_SOF)
            needpullx = storeRegTemp(m6502_reg_x, true);

	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  m6502_emitOp ("asl", "a");
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
          if(needpullx)
            m6502_loadRegTemp(m6502_reg_a);
          else
	    m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);

	  m6502_emitOp ("rol", "a");
	  while(--shCount)
	    {
	      m6502_rmwWithAop ("asl", AOP (result), 0);
	      m6502_emitOp ("rol", "a");
	    }
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
	}
    }
  else
    {
      needpulla = storeRegTempIfSurv (m6502_reg_a);
      needpullx = storeRegTempIfSurv (m6502_reg_x);
      m6502_loadRegFromAop (m6502_reg_xa, AOP (left), 0);
      XAccLsh (m6502_reg_x, shCount);
      m6502_storeRegToFullAop (m6502_reg_xa, AOP (result), 0);
      m6502_loadOrFreeRegTemp (m6502_reg_x, needpullx);
      m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
    }

  if (maskedtopbyte)
    {
      bool in_a = (result->aop->type == AOP_REG && result->aop->aopu.aop_reg[1]->rIdx == A_IDX);
      bool needpull = false;
      if (!in_a)
	{
	  needpull = pushRegIfUsed (m6502_reg_a);
	  m6502_loadRegFromAop (m6502_reg_a, result->aop, 1);
	}
      m6502_emitOp ("and", IMMDFMT, topbytemask);
      if (!in_a)
	{
	  m6502_storeRegToAop (m6502_reg_a, result->aop, 1);
	  pullOrFreeReg (m6502_reg_a, needpull);
	}
    }
 done:
  ;
}

/**************************************************************************
 * shiftLLongInPlace - shift left one long in place
 *
 * @param result pointer to the dst aop
 * @param shift  number of shifts (must be >=0 and <8)
 * @param ofs    LSB to begin the shift (must be >=0 and <4)
 * @param msb_in_a MSB is already in A
 *************************************************************************/
static void
shiftLLongInPlace (operand * result, int shift, int ofs, bool msb_in_a)
{
  int i;

  if(shift==0)
    return;

  if(!msb_in_a && shift==1 /*|| (shift==2 && AOP_TYPE(result)==AOP_DIR) */ )
    {
      while(shift--)
	{
	  m6502_rmwWithAop ("asl", AOP(result), ofs);
	  for(i=ofs+1; i<4; i++)
	    m6502_rmwWithAop ("rol", AOP(result), i);
	}
    }
  else
    {
#if 1
      if(!msb_in_a)
        m6502_loadRegFromAop (m6502_reg_a, AOP (result), 3);
      while(shift)
	{
	  if(ofs<3)
	    m6502_rmwWithAop ("asl", AOP(result), ofs);
	  if(ofs<2)
	    m6502_rmwWithAop ("rol", AOP(result), ofs+1);
	  if(ofs<1)
	    m6502_rmwWithAop ("rol", AOP(result), ofs+2);

	  if(ofs==3)
	    m6502_rmwWithReg ("asl", m6502_reg_a);
	  else
	    m6502_rmwWithReg ("rol", m6502_reg_a);
	  --shift;
	}
      if(!msb_in_a)
        m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);

#else
      if(!msb_in_a)
	m6502_loadRegFromAop (m6502_reg_a, AOP (result), ofs);

      while(shift)
	{
	  m6502_rmwWithReg ("asl", m6502_reg_a);
	  for(i=ofs+1;i<4;i++)
	    m6502_rmwWithAop ("rol", AOP(result), i);

	  --shift;
	}
      if(!msb_in_a)
	m6502_storeRegToAop (m6502_reg_a, AOP (result), ofs);
#endif
    }
}

/**************************************************************************
 * shiftLLong1 - shift left one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >=24 and <32)
 *************************************************************************/
static void
shiftLLong1 (operand * left, operand * result, int shift)
{
  bool needpulla = false;

  wassertl(shift>=24, "shiftLLong1 - shift<24");
  wassertl(shift<32,  "shiftLLong1 - shift>=32");

  needpulla = pushRegIfUsed (m6502_reg_a);

  if(shift==24)
    {
      //      m6502_transferAopAop (AOP (left), 0, AOP (result), 3);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
      m6502_storeConstToAop(0, AOP (result), 0);
      m6502_storeConstToAop(0, AOP (result), 1);
      m6502_storeConstToAop(0, AOP (result), 2);
    } 
  else if(shift==31)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_rmwWithReg ("lsr", m6502_reg_a);
      m6502_loadRegFromConst (m6502_reg_a, 0);
      m6502_storeRegToAop (m6502_reg_a, AOP (result) , 2); // out of order store to save a load
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1); // out of order store to save a load
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0); // out of order store to save a load
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
    }
  else if(shift==30)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_emitOp ("and", IMMDFMT, 0xc0);
      m6502_storeConstToAop (0, AOP (result), 2);
      m6502_storeConstToAop (0, AOP (result), 1);
      m6502_storeConstToAop (0, AOP (result), 0);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
    }
  else
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_storeConstToAop (0, AOP (result), 2);
      m6502_storeConstToAop (0, AOP (result), 1);
      m6502_storeConstToAop (0, AOP (result), 0);
      shiftLLongInPlace (result, shift-24, 3, true);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
    }
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * shiftLLong2 - shift left one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >=16 and <24)
 *************************************************************************/
static void
shiftLLong2 (operand * left, operand * result, int shift)
{
  bool needpulla = false;

  wassertl(shift>=16, "shiftLLong2 - shift<16");
  wassertl(shift<24,  "shiftLLong2 - shift>23");

  needpulla = pushRegIfUsed (m6502_reg_a);

  if(shift==16)
    {
      //     m6502_transferAopAop (AOP (left), 0, AOP (result), 2);
      //     m6502_transferAopAop (AOP (left), 1, AOP (result), 3);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
      m6502_storeConstToAop (0, AOP (result), 1);
      m6502_storeConstToAop (0, AOP (result), 0);
    }
  else if(shift>21)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
      m6502_rmwWithReg ("lsr", m6502_reg_a);
      if(shift==22)
	m6502_pushReg(m6502_reg_a, false);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
      m6502_loadRegFromConst (m6502_reg_a, 0);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1); // out of order store to save a load
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0); // out of order store to save a load
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
      if(shift==22)
	{
	  m6502_pullReg(m6502_reg_a);
	  m6502_rmwWithReg ("lsr", m6502_reg_a);
	  m6502_rmwWithAop ("ror", AOP(result), 3);
	  m6502_rmwWithAop ("ror", AOP(result), 2);
	}
    }
  else
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_rmwWithReg ("asl", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      //m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
      m6502_storeConstToAop (0, AOP (result), 1);
      m6502_storeConstToAop (0, AOP (result), 0);
      shiftLLongInPlace (result, shift-17, 2, true);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
    }

  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * shiftLLong3 - shift left one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >=8 and <16)
 *************************************************************************/
static void
shiftLLong3 (operand * left, operand * result, int shift)
{
  bool needpulla = false;
  bool needloadx = false;

  wassertl(shift>=8, "shiftLLong3 - shift<8");
  wassertl(shift<16,  "shiftLLong3 - shift>=16");

  //  if(shift==8 && AOP_TYPE(left)!=AOP_SOF &&  AOP_TYPE(result)!=AOP_SOF)
  //    reg=m6502_getFreeByteReg();
  //  if(!reg)
  //    reg=m6502_reg_a;

  needpulla = pushRegIfUsed (m6502_reg_a);

  //  idx=getFreeIdxReg();
  //  if(!idx)
  //    idx=m6502_reg_x;

  if(shift==8)
    {
      //      m6502_transferAopAop (AOP (left), 2, AOP (result), 3);
      //      m6502_transferAopAop (AOP (left), 1, AOP (result), 2);
      //      m6502_transferAopAop (AOP (left), 0, AOP (result), 1);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      m6502_freeReg(m6502_reg_a);
      m6502_storeConstToAop (0, AOP (result), 0);
    }
  else if(shift>13)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
      m6502_rmwWithReg ("lsr", m6502_reg_a);
      if(shift==14)
	m6502_pushReg(m6502_reg_a, false);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
      m6502_loadRegFromConst (m6502_reg_a, 0);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0); // out of order store to save a load
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      if(shift==14)
	{
	  m6502_pullReg(m6502_reg_a);
	  m6502_rmwWithReg ("lsr", m6502_reg_a);
	  m6502_rmwWithAop ("ror", AOP(result), 3);
	  m6502_rmwWithAop ("ror", AOP(result), 2);
	  m6502_rmwWithAop ("ror", AOP(result), 1);
	}
    }
  else 
    {
      if(!m6502_sameRegs (AOP (left), AOP (result)))
	{ 
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  m6502_rmwWithReg ("asl", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  //	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	  m6502_storeConstToAop (0, AOP (result), 0);
	}
      else
	{
	  needloadx = storeRegTempIfUsed (m6502_reg_x);

	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  m6502_rmwWithReg ("asl", m6502_reg_a);
	  m6502_loadRegFromAop (m6502_reg_x, AOP (left), 1);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_transferRegReg(m6502_reg_x, m6502_reg_a, true);
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  m6502_loadRegFromAop (m6502_reg_x, AOP (left), 2);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);

	  m6502_transferRegReg(m6502_reg_x, m6502_reg_a, true);
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  //	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	  m6502_storeConstToAop (0, AOP (result), 0);
	}
      shiftLLongInPlace (result, shift-9, 1, true);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);

    }
  m6502_loadOrFreeRegTemp (m6502_reg_x, needloadx);
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * shiftLLong4 - shift left one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >0 and <8)
 *************************************************************************/
static void
shiftLLong4 (operand * left, operand * result, int shift)
{
  bool needpulla = false;

  wassertl(shift>0, "shiftLLong4 - shift==0");
  wassertl(shift<8, "shiftLLong4 - shift>=8");

  needpulla = pushRegIfUsed (m6502_reg_a);

  if(shift>5)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      m6502_rmwWithReg ("lsr", m6502_reg_a);
      if(shift!=7)
	m6502_fastSaveA();

      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      m6502_loadRegFromConst (m6502_reg_a, 0);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      if(shift!=7)
	{
	  m6502_fastRestoreA();
	  while(shift!=7)
	    {
	      m6502_rmwWithReg ("lsr", m6502_reg_a);
	      //           m6502_emitRegTempOp("lsr", m6502_getLastTempOfs() );
	      m6502_rmwWithAop ("ror", AOP(result), 3);
	      m6502_rmwWithAop ("ror", AOP(result), 2);
	      m6502_rmwWithAop ("ror", AOP(result), 1);
	      m6502_rmwWithAop ("ror", AOP(result), 0);
              shift++;
	    }
	  //           m6502_loadRegTemp(NULL);
	}
    }
  else
    {
      if(!m6502_sameRegs (AOP (left), AOP (result)))
	{ 
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  m6502_rmwWithReg ("asl", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  //	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
          shiftLLongInPlace (result, shift-1, 0, true);
          m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	}
      else
        {
	  //  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
          shiftLLongInPlace (result, shift, 0, false);
	  //  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
        }
    }
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * genlsh32 - shift four byte by a known amount != 0
 *************************************************************************/
static void
genlsh32 (operand * result, operand * left, int shCount)
{
  m6502_emitComment (TRACEGEN, "  %s - shift=%d", __func__, shCount);

  sym_link *resulttype = operandType (result);
  unsigned topbytemask = (IS_BITINT (resulttype) && SPEC_USIGN (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;
  bool maskedtopbyte = (topbytemask != 0xff);

  wassertl(AOP_SIZE (result)==4, "genlsh32 - AOP_SIZE != 4");

  /* if shifting more that 3 bytes */
  if (shCount >= 24)
    {
      shiftLLong1 (left, result, shCount);
    }
  else if (shCount >= 16)
    {
      shiftLLong2 (left, result, shCount);
    }
  else if (shCount >= 8)
    {
      shiftLLong3 (left, result, shCount);
    }
  else
    {
      shiftLLong4 (left, result, shCount);
    }
  
  if (maskedtopbyte)
    {
      bool in_a = (result->aop->type == AOP_REG && result->aop->aopu.aop_reg[1]->rIdx == A_IDX);
      bool needpull = false;
      if (!in_a)
        {
          needpull = pushRegIfUsed (m6502_reg_a);
          m6502_loadRegFromAop (m6502_reg_a, result->aop, 1);
        }
      m6502_emitOp ("and", IMMDFMT, topbytemask);
      if (!in_a)
        {
          m6502_storeRegToAop (m6502_reg_a, result->aop, 1);
          pullOrFreeReg (m6502_reg_a, needpull);
        }
    }
}

/**************************************************************************
 * genLeftShiftLiteral - left shifting by known count
 *************************************************************************/
static void
genLeftShiftLiteral (operand * left, operand * result, int shCount)
{
  bool restore_x = false;
  int size, offset;

  m6502_emitComment (TRACEGEN, __func__);

  size = AOP_SIZE (result);
  m6502_emitComment (TRACEGEN|VVDBG, "  %s - result size=%d, left size=%d",
		     __func__, size, AOP_SIZE (left));

  if (shCount == 0)
    {
      m6502_copy (result, left);
    }
  else if (shCount >= (size * 8))
    {
      for(offset=0;offset<size; offset++)
        m6502_storeConstToAop (0, AOP (result), offset);
    } 
  else
    {
      // FIXME: should move this to each genlsh
      if(AOP_TYPE(left)==AOP_SOF || AOP_TYPE(result)==AOP_SOF)
	restore_x=storeRegTempIfSurv(m6502_reg_x);

      switch (size)
	{
	case 1:
	  genlsh8 (result, left, shCount);
	  break;
	case 2:
	  genlsh16 (result, left, shCount);
	  break;
	case 4:
	  genlsh32 (result, left, shCount);
	  break;
	default:
	  emitcode("ERROR", "%s: Invalid operand size %d", __func__, size);
	  break;
	}
      m6502_loadOrFreeRegTemp(m6502_reg_x, restore_x);
    }
}

/**************************************************************************
 * genLeftShift - generates code for left shifting
 *************************************************************************/
void
m6502_genLeftShift (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int size, offset;
  symbol *loop_label, *skip_label;
  reg_info *countreg = NULL;
  bool restore_a = false;
  bool restore_y = false;

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (right, ic);
  m6502_aopOp (left, ic);
  m6502_aopOp (result, ic);

  m6502_printIC(ic);

  sym_link *resulttype = operandType (result);
  unsigned topbytemask = (IS_BITINT (resulttype) && SPEC_USIGN (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;
  bool maskedtopbyte = (topbytemask != 0xff);

  /* if the shift count is known then do it
     as efficiently as possible */
  if (AOP_TYPE (right) == AOP_LIT &&
      (getSize (operandType (result)) == 1 || getSize (operandType (result)) == 2 || getSize (operandType (result)) == 4))
    {
      int shCount = (int) ulFromVal (AOP (right)->aopu.aop_lit);
      genLeftShiftLiteral (left, result, shCount);
      goto release;
    }

  /* shift count is unknown then we have to form
     a loop get the loop count in X : Note: we take
     only the lower order byte since shifting
     more that 64 bits make no sense anyway, ( the
     largest size of an object can be only 64 bits ) */

  // TODO
#if 0
  if (m6502_sameRegs (AOP (right), AOP (result)) || regsInCommon (right, result) || IS_AOP_XA (AOP (result)) || isOperandVolatile (result, false))
    aopResult = forceZeropageAop (AOP (result), m6502_sameRegs (AOP (left), AOP (result)));
#endif

  size = AOP_SIZE (result);
  loop_label = m6502_safeNewiTempLabel (NULL);
  skip_label = m6502_safeNewiTempLabel (NULL);

  if (!m6502_reg_a->isDead && !IS_AOP_WITH_A (AOP (result)))
    {
      storeRegTemp(m6502_reg_a, true);
      restore_a=true;
    }

  bool src_x = ( IS_AOP_XA (AOP (left)) || IS_AOP_XY (AOP (left)) );
  bool dst_x = ( IS_AOP_XA (AOP (result)) || IS_AOP_XY (AOP (result)) );
  bool late_x = false;

  bool msb_in_x = (src_x || dst_x) && AOP_TYPE(result)!=AOP_DIR;


  bool early_load_count = (AOP_TYPE(left)==AOP_SOF || AOP_TYPE(right)==AOP_SOF
			   || IS_AOP_WITH_A(AOP(right)) || m6502_sameRegs (AOP(result), AOP(right)) );

  int a_loc = ( src_x | dst_x )? 0 : size-1;

  /* find a count register */
  if (m6502_reg_x->isDead && IS_AOP_X (AOP (right))
      && AOP_TYPE(left)!=AOP_SOF && AOP_TYPE(result)!=AOP_SOF )
    countreg = m6502_reg_x;
  else if (m6502_reg_y->isDead && !(IS_AOP_WITH_A (AOP (right)) && IS_AOP_WITH_Y (AOP (left)) ) )
    countreg = m6502_reg_y;
  else if (m6502_reg_x->isDead && !IS_AOP_WITH_X (AOP (result)) && !IS_AOP_WITH_X (AOP (left))
           && AOP_TYPE(left)!=AOP_SOF && AOP_TYPE(result)!=AOP_SOF )
    countreg = m6502_reg_x;
  else //if (!IS_AOP_WITH_Y (AOP (result)) && !IS_AOP_WITH_Y (AOP (left)))
    {
      // Y is live
      //	emitcode("ERROR", "%s: countreg is null", __func__);
      if(!m6502_reg_y->isDead && !IS_AOP_WITH_Y (AOP (result)))
        {
	  storeRegTemp(m6502_reg_y, !IS_AOP_WITH_Y (AOP (right)));
	  restore_y=true;
        }
      countreg = m6502_reg_y;
    }

  m6502_emitComment (TRACEGEN, "  %s - enter size:%d src_x:%d dst_x:%d tmsb:%d countreg:%s",
		     __func__, size, src_x, dst_x, msb_in_x, countreg->name);

  if(size==1)
    {
      if(IS_AOP_Y(AOP(left)) && IS_AOP_A(AOP(right)) && countreg==m6502_reg_x)
        early_load_count = true;
      else if(IS_AOP_Y(AOP(left)))
        early_load_count = false;
    }

  if(IS_AOP_XY(AOP(left)))
    early_load_count = false;

  if(early_load_count)
    {
      m6502_emitComment (TRACEGEN, "  %s - early count", __func__);
      bool save_x = false;
      bool save_a = false;

      if(AOP_TYPE(right)==AOP_SOF)
        {
          save_x=storeRegTempIfUsed(m6502_reg_x);
          save_a=storeRegTempIfUsed(m6502_reg_a);
        }

      m6502_loadRegFromAop (countreg, AOP (right), 0);
      if(IS_AOP_WITH_A(AOP(right)))
        m6502_reg_a->isFree=true;
      if(IS_AOP_WITH_X(AOP(right)))
        m6502_reg_x->isFree=true;
      if(save_a)
        m6502_loadRegTemp(m6502_reg_a);
      if(save_x)
        m6502_loadRegTemp(m6502_reg_x);

    }

  if(src_x)
    {
      m6502_emitComment (TRACEGEN, "  %s - src op has x", __func__);
      
      late_x = src_x && dst_x && !IS_AOP_A (AOP (right)) && AOP_TYPE(right)!=AOP_SOF;  

      if(msb_in_x)
        {
          if(!late_x)
	    {
	      storeRegTempAlways(m6502_reg_x, true);
	      m6502_dirtyRegTemp (m6502_getLastTempOfs());
	    }
        }
      else
        m6502_transferAopAop (AOP (left), 1, AOP (result), 1);

      if(IS_AOP_A (AOP (right)))
	{
	  countreg=m6502_reg_x;
	  m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);
	  early_load_count=true;
	}

      if(IS_AOP_XY (AOP (left)))
        m6502_transferRegReg(m6502_reg_y, m6502_reg_a, true);

    }
  else if(dst_x)
    {
      m6502_emitComment (TRACEGEN, "  %s - dst op has x", __func__);
      m6502_loadRegFromAop (m6502_reg_xa, AOP (left), 0);
      late_x = true;
    }
  else if (!m6502_sameRegs (AOP (left), AOP (result)))
    {
      for (offset=0; offset<size-1; offset++)
	m6502_transferAopAop (AOP (left), offset, AOP (result), offset);

      m6502_loadRegFromAop (m6502_reg_a, AOP (left), a_loc);
    }
  else
    m6502_loadRegFromAop (m6502_reg_a, AOP (left), a_loc);


  if(!early_load_count)
    {
      m6502_emitComment (TRACEGEN, "%s: late countreg", __func__);
      m6502_loadRegFromAop (countreg, AOP (right), 0);
    }

  m6502_useReg (countreg);
  if(IS_AOP_XA(AOP(right)) || IS_AOP_XY(AOP(right)))
    m6502_freeReg(m6502_reg_x);

  // FIXME: make this conditional on opt code-speed
  if(size==8 /*|| size==4*/)
    {
      symbol *skiplbl = m6502_safeNewiTempLabel (NULL);
      symbol *looplbl = m6502_safeNewiTempLabel (NULL);

      m6502_emitCmp(countreg, 8);
      m6502_emitBranch ("bcc", skiplbl);
      m6502_safeEmitLabel (looplbl);
      m6502_dirtyAllRegs();

      if(size==8)
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (result), 6);
	  m6502_storeRegToAop (m6502_reg_a, AOP(result) , 7);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (result), 5);
	  m6502_storeRegToAop (m6502_reg_a, AOP(result) , 6);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (result), 4);
	  m6502_storeRegToAop (m6502_reg_a, AOP(result) , 5);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (result), 3);
	  m6502_storeRegToAop (m6502_reg_a, AOP(result) , 4);
	}

      m6502_loadRegFromAop (m6502_reg_a, AOP (result), 2);
      m6502_storeRegToAop (m6502_reg_a, AOP(result) , 3);
      m6502_loadRegFromAop (m6502_reg_a, AOP (result), 1);
      m6502_storeRegToAop (m6502_reg_a, AOP(result) , 2);
      m6502_loadRegFromAop (m6502_reg_a, AOP (result), 0);
      m6502_storeRegToAop (m6502_reg_a, AOP(result) , 1);

      m6502_loadRegFromConst (m6502_reg_a, 0);

      m6502_storeRegToAop (m6502_reg_a, AOP(result), 0);


      m6502_transferRegReg(countreg, m6502_reg_a, true);
      m6502_emitSetCarry (1);
      m6502_emitOp ("sbc", IMMDFMT, 8);
      m6502_transferRegReg(m6502_reg_a, countreg, true);
      //if(size==8)
      {
	m6502_emitCmp(countreg, 8);
	m6502_emitBranch ("bcs", looplbl);
      }
      m6502_loadRegFromAop (m6502_reg_a, AOP (result), a_loc);
      m6502_safeEmitLabel (skiplbl);
    }

  m6502_emitCmp(countreg, 0);
  m6502_emitBranch ("beq", skip_label);

  if(msb_in_x && late_x)
    {
      storeRegTempAlways(m6502_reg_x, true);
      m6502_dirtyRegTemp (m6502_getLastTempOfs());
    }

  // FIXME: find a good solution for this
  //  if(IS_AOP_WITH_A (AOP (right)) && m6502_sameRegs (AOP (left), AOP (result)) )
  //    m6502_loadRegFromAop (m6502_reg_a, AOP (left), a_loc);

  m6502_safeEmitLabel (loop_label); // loop label

  if(a_loc==0)
    {
      m6502_emitComment (TRACEGEN, "  %s - aloc==0", __func__);
      m6502_rmwWithReg ("asl", m6502_reg_a);
      if(size==2)
        {
	  if(msb_in_x)
	    m6502_emitRegTempOp("rol", m6502_getLastTempOfs() );
	  else
	    m6502_rmwWithAop ("rol", AOP (result), 1);
        }
    }
  else
    {
      m6502_rmwWithAop ("asl", AOP (result), 0);

      for (offset = 1; offset < size-1; offset++)
	m6502_rmwWithAop ("rol", AOP (result), offset);

      m6502_rmwWithReg ("rol", m6502_reg_a);
    }

  m6502_rmwWithReg("dec", countreg);
  m6502_emitBranch ("bne", loop_label);

  if (msb_in_x && countreg!=m6502_reg_x)
    m6502_loadRegTemp(m6502_reg_x);

  m6502_safeEmitLabel (skip_label); // end label

  if (msb_in_x && countreg==m6502_reg_x)
    m6502_loadRegTemp(m6502_reg_x);

  if (maskedtopbyte)
    {

      // FIXME:   m6502_storeRegToAop (m6502_reg_a, AOP(result), a_loc);
      // use this to avoid pha/pla

      if(src_x || dst_x)
        {
          m6502_pushReg (m6502_reg_a, false);
	  if(msb_in_x)
	    m6502_transferRegReg(m6502_reg_x, m6502_reg_a, false);
	  else
	    m6502_loadRegFromAop (m6502_reg_a, AOP (result), 1);

          m6502_emitOp ("and", IMMDFMT, topbytemask);

	  if(msb_in_x)
	    m6502_transferRegReg(m6502_reg_a, m6502_reg_x, false);
	  else
            m6502_storeRegToAop (m6502_reg_a, AOP(result), 1);

          m6502_pullReg (m6502_reg_a);
        }
      else
        m6502_emitOp ("and", IMMDFMT, topbytemask);
    }

  m6502_storeRegToAop (m6502_reg_a, AOP(result), a_loc);

  if(msb_in_x)
    m6502_storeRegToAop (m6502_reg_x, AOP(result), 1);

  if(IS_AOP_WITH_REG(AOP(result), countreg))
    {
      m6502_dirtyReg(countreg);
    }
  else
    {
      // After loop, countreg is always 0
      m6502_dirtyReg(countreg);
      countreg->isLitConst = 1;
      countreg->litConst = 0;
    }

  if(restore_y)
    m6502_loadRegTemp(m6502_reg_y);
  if(restore_a)
    m6502_loadRegTemp(m6502_reg_a);

 release:
  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (result, NULL);
}

