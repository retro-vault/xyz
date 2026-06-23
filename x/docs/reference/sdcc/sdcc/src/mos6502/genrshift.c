/*-------------------------------------------------------------------------
  genrshift.c - source file for right shift code generation for the MOS6502

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
 * AccSRsh - signed right shift accumulator by known count
 *************************************************************************/
static void
AccSRsh (int shCount)
{
  int i;

  shCount &= 0x0007;            // shCount : 0..7

  if (shCount == 7)
    {
      m6502_emitOp ("rol", "a");
      m6502_loadRegFromConst(m6502_reg_a, 0);
      m6502_emitOp ("adc", "#0xff");
      m6502_emitOp ("eor", "#0xff");
    }
  else if (shCount == 6)
    {
      symbol *tlbl = m6502_safeNewiTempLabel (NULL);
      m6502_emitOp ("ora", "#0x3f");
      m6502_emitSetCarry(1);
      m6502_emitOp ("bmi", "%05d$", m6502_safeLabelNum (tlbl));
      m6502_emitOp ("and", "#0xc0");
      m6502_emitSetCarry(0);
      m6502_safeEmitLabel(tlbl);
      m6502_emitOp ("rol", "a");
      m6502_emitOp ("rol", "a");
      m6502_emitOp ("rol", "a");
    }
  else
    {
      // TODO: optimize?
      for (i = 0; i < shCount; i++)
        {
          m6502_emitCmp (m6502_reg_a, 0x80);
          m6502_emitOp ("ror", "a");
        }
    }
}

/**************************************************************************
 * m6502_AccRsh - right shift accumulator by known count
 *************************************************************************/
void
m6502_AccRsh (int shCount, bool sign)
{
  int i;

  if (sign) {
    AccSRsh (shCount);
    return;
  }

  shCount &= 0x0007;            // shCount : 0..7

  /* For shift counts of 6 and 7, the unrolled loop is never optimal.      */
  if (shCount==7)
    {
      m6502_emitOp ("rol", "a");
      m6502_loadRegFromConst(m6502_reg_a, 0);
      m6502_emitOp ("rol", "a");
    }
  else if(shCount==6)
    {
      m6502_emitOp ("rol", "a");
      m6502_emitOp ("rol", "a");
      m6502_emitOp ("rol", "a");
      m6502_emitOp ("and", "#0x03");
    }
  else
    {
      /* lsr a is 2 cycles and 1 byte, so an unrolled loop is the      */
      /* fastest and shortest (shCount<6).                             */
      for (i = 0; i < shCount; i++)
        m6502_emitOp ("lsr", "a");
    }
}

/**************************************************************************
 * XAccSRsh - signed right shift register pair XA by known count
 *************************************************************************/
static void
XAccSRsh (int shCount)
{
  symbol *tlbl;
  int i;

  shCount &= 0x000f;            // shCount : 0..7

  /* if we can beat 2n cycles or bytes for some special case, do it here */
  switch (shCount) { // TODO
  case 15:
  case 14:
  case 13:
  case 12:
  case 11:
  case 10:
  case 9:
    m6502_transferRegReg (m6502_reg_x, m6502_reg_a, false);
    m6502_loadRegFromConst (m6502_reg_x, 0);
    AccSRsh (shCount - 8);
    tlbl = m6502_safeNewiTempLabel (NULL);
    m6502_emitOp ("bpl", "%05d$", m6502_safeLabelNum (tlbl));
    m6502_loadRegFromConst (m6502_reg_x, 0xff);
    m6502_safeEmitLabel(tlbl);
    break;
  case 8:
    m6502_transferRegReg (m6502_reg_x, m6502_reg_a, false);
    m6502_loadRegFromConst (m6502_reg_x, 0);
    m6502_emitCmp (m6502_reg_a, 0x00);
    tlbl = m6502_safeNewiTempLabel (NULL);
    m6502_emitOp ("bpl", "%05d$", m6502_safeLabelNum (tlbl));
    m6502_loadRegFromConst (m6502_reg_x, 0xff);
    m6502_safeEmitLabel(tlbl);
    break;

  default:
    // TODO: handle when X is litconst
    storeRegTempAlways(m6502_reg_x, true);
    for (i = 0; i < shCount; i++)
      {
	// TODO: see if theree is a better way
	m6502_emitOp ("cpx", "#0x80");
	m6502_emitRegTempOp ("ror", m6502_getLastTempOfs() );
	m6502_dirtyRegTemp(m6502_getLastTempOfs() );
	m6502_rmwWithReg ("ror", m6502_reg_a);
      }
    m6502_loadRegTemp(m6502_reg_x);
  }
}

/**************************************************************************
 * XAccRsh - right shift register pair XA by known count
 *************************************************************************/
void
XAccRsh (int shCount, bool sign)
{
  int i;

  shCount &= 0x000f;            // shCount : 0..15

  if(m6502_reg_x->isLitConst)
    {
      unsigned long v = (m6502_reg_x->litConst<<8);

      if(sign && (v&0x8000))
        v|=0xffff0000;

      v>>=shCount;

      if(shCount<8)
        {
          m6502_AccRsh (shCount, false);
          if(v&0xff)
            m6502_emitOp ("ora", IMMDFMT, v&0xff);
        }
      else
        m6502_loadRegFromConst(m6502_reg_a, v&0xff);

      m6502_loadRegFromConst(m6502_reg_x, (v>>8)&0xff);
      return;
    }
    
  if (sign)
    {
      XAccSRsh (shCount);
      return;
    }

  if(shCount>=8)
    {
      m6502_transferRegReg(m6502_reg_x, m6502_reg_a, true);
      m6502_AccRsh (shCount - 8, false);
      m6502_loadRegFromConst (m6502_reg_x, 0);
    }
  else if(shCount==7)
    {
      storeRegTempAlways(m6502_reg_x, true);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      m6502_emitRegTempOp ("rol", m6502_getLastTempOfs() );
      m6502_dirtyRegTemp(m6502_getLastTempOfs() );
      m6502_loadRegFromConst (m6502_reg_a, 0);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);
      m6502_loadRegTemp(m6502_reg_a);
    }
  else if(shCount!=0)
    {
      /* lsr/rora is only 2 cycles and bytes, so an unrolled loop is often  */
      /* the fastest and shortest.                                           */
      storeRegTempAlways(m6502_reg_x, true);
      for (i = 0; i < shCount; i++)
        {
          m6502_emitRegTempOp ("lsr", m6502_getLastTempOfs() );
          m6502_rmwWithReg ("ror", m6502_reg_a);
          m6502_dirtyRegTemp(m6502_getLastTempOfs() );
        }
      m6502_loadRegTemp(m6502_reg_x);
    }
}

/**************************************************************************
 * genrsh8 - right shift a one byte quantity by known count
 *************************************************************************/
static void
genrsh8 (operand * result, operand * left, int shCount, int sign)
{
  bool needpulla = false;

  m6502_emitComment (TRACEGEN, "  %s - shift=%d", __func__, shCount);
  if (shCount==0)
    return;

  if (!IS_AOP_A(AOP(result)) && m6502_sameRegs (AOP (left), AOP (result)) 
      && shCount<3 && m6502_aopCanShift(AOP(left)) && !sign)
    {
      while (shCount--)
        m6502_rmwWithAop ("lsr", AOP (result), 0);
    }
  else
    {
      if(!IS_AOP_A(AOP(result)))
	needpulla = pushRegIfSurv (m6502_reg_a);

      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_AccRsh (shCount, sign);
      m6502_storeRegToFullAop (m6502_reg_a, AOP (result), sign);
      pullOrFreeReg (m6502_reg_a, needpulla);
    }
}

/**************************************************************************
 * genrsh16 - right shift two bytes by known amount != 0
 *************************************************************************/
static void
genrsh16 (operand * result, operand * left, int shCount, int sign)
{
  bool needpulla = false;
  bool needpullx = false;

  m6502_emitComment (TRACEGEN, "  %s - shift=%d", __func__, shCount);

  if (shCount >= 8)
    {
      if (shCount != 8 || sign)
	{
	  needpulla = pushRegIfSurv (m6502_reg_a);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  m6502_AccRsh (shCount - 8, sign);
	  m6502_storeRegToFullAop (m6502_reg_a, AOP (result), sign);
	  pullOrFreeReg (m6502_reg_a, needpulla);
	}
      else
	{
	  m6502_transferAopAop (AOP (left), 1, AOP (result), 0);
	  m6502_storeConstToAop (0, AOP (result), 1);
	}
    }
  else if(IS_AOP_XA(AOP(result)))
    {
      if(shCount==1 && !IS_AOP_XA(AOP(left)))
        {
          m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
          if(sign)
            {
	      m6502_emitCmp (m6502_reg_a, 0x80);
	      m6502_emitOp ("ror", "a");
            }
          else 
	    m6502_emitOp ("lsr", "a");
          if(AOP_TYPE(left)==AOP_SOF)
            {
              storeRegTempAlways (m6502_reg_a, true);
              m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	      m6502_emitOp ("ror", "a");
	      m6502_loadRegTemp (m6502_reg_x);
            }
          else
            {
              m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);
              m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	      m6502_emitOp ("ror", "a");
            }
        }
      else
	{
	  /*  1 <= shCount <= 7 */
	  // TODO: count > 2 efficient?
	  m6502_loadRegFromAop (m6502_reg_xa, AOP (left), 0);
	  XAccRsh (shCount, sign);
        }
    }
  else
    {
      needpulla = storeRegTempIfSurv (m6502_reg_a);
      needpullx = storeRegTempIfSurv (m6502_reg_x);
      m6502_loadRegFromAop (m6502_reg_xa, AOP (left), 0);
      XAccRsh (shCount, sign);
      m6502_storeRegToAop (m6502_reg_xa, AOP (result), 0);
      m6502_loadOrFreeRegTemp (m6502_reg_x, needpullx);
      m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
    }
}

/**************************************************************************
 * shiftRLongInPlace - shift left one long in place
 *
 * @param result pointer to the dst aop
 * @param shift  number of shifts (must be >=0 and <8)
 * @param ofs    LSB to begin the shift (must be >=0 and <4)
 * @param msb_in_a MSB is already in A
 *************************************************************************/
static void
shiftRLongInPlace (operand * result, int shift, int ofs, int sign, bool msb_in_a)
{
  int i;

  if(shift==0)
    return;

#if 0
  if(!msb_in_a && shift==1 && !sign /*|| (shift==2 && AOP_TYPE(result)==AOP_DIR) */ )
    {
      while(shift--)
	{
	  m6502_rmwWithAop ("lsr", AOP(result), 3-ofs);
          for(i=2-ofs; i>=0; i--)
	    m6502_rmwWithAop ("ror", AOP(result), i);
	}
      else
	{

	  if(!msb_in_a)
	    m6502_loadRegFromAop (m6502_reg_a, AOP (result), 0);
	  while(shift)
	    {
	      if(ofs<3)
		m6502_rmwWithAop ("lsr", AOP(result), ofs);
	      if(ofs<2)
		m6502_rmwWithAop ("ror", AOP(result), ofs+1);
	      if(ofs<1)
		m6502_rmwWithAop ("ror", AOP(result), ofs+2);

	      if(ofs==3)
		m6502_rmwWithReg ("lsr", m6502_reg_a);
	      else
		m6502_rmwWithReg ("ror", m6502_reg_a);

	      --shift;
	    }
	  if(!msb_in_a)
	    m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
	}
    }
#else
  if(!msb_in_a)
    m6502_loadRegFromAop (m6502_reg_a, AOP (result), 3-ofs);

  while(shift)
    {
      if(sign)
	{
	  m6502_emitCmp(m6502_reg_a, 0x80);
	  m6502_rmwWithReg ("ror", m6502_reg_a);
	}
      else
	m6502_rmwWithReg ("lsr", m6502_reg_a);

      for(i=2-ofs;i>=0;i--)
	m6502_rmwWithAop ("ror", AOP(result), i);

      --shift;
    }
  if(!msb_in_a)
    m6502_storeRegToAop (m6502_reg_a, AOP (result), 3-ofs);
#endif
}

/**************************************************************************
 * shiftRLong1 - shift right one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >=24 and <32)
 *************************************************************************/
static void
shiftRLong1 (operand * left, operand * result, int shift, int sign)
{
  bool needpulla = false;

  wassertl(shift>=24, "shiftRLong1 - shift<24");
  wassertl(shift<32, "shiftRLong1 - shift>=32");

  needpulla = pushRegIfUsed (m6502_reg_a);

  if(shift==24)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      //	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      shift=0;
    }
  else if (shift>29)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      m6502_emitOp ("asl", "a");

      if(shift!=31)
	storeRegTemp(m6502_reg_a, true);
      if(sign)
	{
	  //	  m6502_signExtendReg(m6502_reg_a);
	  //  m6502_emitOp ("asl", "a");
	  m6502_loadRegFromConst (m6502_reg_a, 0);
	  m6502_emitOp ("adc", "#0xff");
	  m6502_emitOp ("eor", "#0xff");
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	}
      else
	{
	  //	  m6502_rmwWithReg ("asl", m6502_reg_a);
	  m6502_loadRegFromConst(m6502_reg_a, 0);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
	}

      if(shift!=31)
        {
	  while(shift!=31)
	    {
	      m6502_emitRegTempOp( "rol", m6502_getLastTempOfs() );
	      m6502_rmwWithAop ("rol", AOP (result), 0);
              shift++;
	    }
	  m6502_loadRegTemp(NULL);
        }

      goto release;
    }
  else
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      if(sign)
	{
	  m6502_emitCmp(m6502_reg_a, 0x80);     
	  m6502_rmwWithReg ("ror", m6502_reg_a);
        }
      else
	m6502_rmwWithReg ("lsr", m6502_reg_a);

      shift-=25;
    }

  shiftRLongInPlace (result, shift, 3, sign, true);
  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);

  if(sign)
    {
      m6502_signExtendReg(m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
    }
  else
    {
      m6502_storeConstToAop (0, AOP (result), 1);
      m6502_storeConstToAop (0, AOP (result), 2);
      m6502_storeConstToAop (0, AOP (result), 3);
    }

 release:
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * shiftRLong2 - shift right one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >=16 and <24)
 *************************************************************************/
static void
shiftRLong2 (operand * left, operand * result, int shift, int sign)
{
  bool needpulla = false;

  wassertl(shift>=16, "shiftRLong3 - shift<16");
  wassertl(shift<24, "shiftRLong3 - shift>23");

  needpulla = pushRegIfUsed (m6502_reg_a);

  if(shift==16)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      shift=0;
    }
  else if(shift>20)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      if(shift!=23)
	storeRegTemp(m6502_reg_a, true);

      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      m6502_loadRegFromConst (m6502_reg_a, 0);
      if(sign)
	{
	  m6502_emitOp ("adc", "#0xff");
	  m6502_emitOp ("eor", "#0xff");
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	}
      else
	{
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
          m6502_rmwWithReg ("rol", m6502_reg_a);
          m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	}

      if(shift!=23)
        {
	  while(shift!=23)
	    {
	      m6502_emitRegTempOp( "rol", m6502_getLastTempOfs() );
	      m6502_rmwWithAop ("rol", AOP (result), 0);
	      m6502_rmwWithAop ("rol", AOP (result), 1);
              shift++;
	    }
	  m6502_loadRegTemp(NULL);
        }

      goto release;
    }
  else
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      if(sign)
	{
	  m6502_emitCmp(m6502_reg_a, 0x80);
	  m6502_rmwWithReg ("ror", m6502_reg_a);
	}
      else
	m6502_rmwWithReg ("lsr", m6502_reg_a);

      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);

      shift-=17;
      m6502_loadRegFromAop (m6502_reg_a, AOP (result), 1);
    }

  shiftRLongInPlace (result, shift, 2, sign, false);

  if(sign)
    {
      m6502_signExtendReg(m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);

    }
  else
    {
      m6502_storeConstToAop (0, AOP (result), 2);
      m6502_storeConstToAop (0, AOP (result), 3);
    }

 release:
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * shiftRLong3 - shift right one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >=8 and <16)
 *************************************************************************/
static void
shiftRLong3 (operand * left, operand * result, int shift, int sign)
{
  bool needpulla = false;
  bool needloadx = false;

  wassertl(shift>=8, "shiftRLong3 - shift<8");
  wassertl(shift<16, "shiftRLong3 - shift>15");

  needpulla = pushRegIfUsed (m6502_reg_a);

  if(shift==8)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
      shift=0;
    }
  else if(shift>12)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      if(shift!=15)
	storeRegTemp(m6502_reg_a, true);

      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      if(sign)
	{
	  m6502_loadRegFromConst(m6502_reg_a, 0);
	  m6502_emitOp("adc", "#0xff");
	  m6502_emitOp("eor", "#0xff");
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	}
      else
	{
          m6502_loadRegFromConst(m6502_reg_a, 0);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3); // out of order store
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	}

      if(shift!=15)
        {
	  while(shift!=15)
	    {
	      m6502_emitRegTempOp( "rol", m6502_getLastTempOfs() );
	      m6502_rmwWithAop ("rol", AOP (result), 0);
	      m6502_rmwWithAop ("rol", AOP (result), 1);
	      m6502_rmwWithAop ("rol", AOP (result), 2);
              shift++;
	    }
	  m6502_loadRegTemp(NULL);
        }

      goto release;
    }
  else if (shift==9 && m6502_sameRegs (AOP (left), AOP (result)) && sign)
    {
      needloadx = storeRegTempIfUsed (m6502_reg_x);
      m6502_loadRegFromConst(m6502_reg_x, 0);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      //            if(sign)
      //              {
      symbol *tlbl = m6502_safeNewiTempLabel (NULL);

      m6502_emitCmp(m6502_reg_a, 0x80);
      m6502_emitBranch ("bcc", tlbl);
      m6502_rmwWithReg ("dec", m6502_reg_x);
      m6502_safeEmitLabel(tlbl);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_x, AOP (result), 3);
      //              }
      //            else
      //	      m6502_rmwWithReg ("lsr", m6502_reg_a);

      m6502_loadRegFromAop (m6502_reg_x, AOP (left), 2);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
      m6502_transferRegReg(m6502_reg_x, m6502_reg_a, true);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_loadRegFromAop (m6502_reg_x, AOP (left), 1);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      m6502_transferRegReg(m6502_reg_x, m6502_reg_a, true);
      m6502_rmwWithReg ("ror", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);

      goto release;
    }
  else
    {
      if(!m6502_sameRegs (AOP (left), AOP (result)) && shift==12)
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
	  m6502_rmwWithReg ("lsr", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
          m6502_rmwWithReg ("ror", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
          m6502_rmwWithReg ("ror", m6502_reg_a);
	  //	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
          shift-=9;
	  for(int i=0;i<shift;i++)
	    {
	      m6502_rmwWithAop ("lsr", AOP(result), 2);
	      m6502_rmwWithAop ("ror", AOP(result), 1);
	      m6502_rmwWithReg ("ror", m6502_reg_a);
	    }
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);

          if(sign)
	    {
	      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
	      m6502_signExtendReg(m6502_reg_a);
	      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	      m6502_emitOp("and", "#0xf0");
	      m6502_rmwWithAop ("ora", AOP(result), 2);
	      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);          
	    }
          else
	    {
	      m6502_storeConstToAop (0, AOP (result), 3);
	    }
	  goto release;
	}
      else if(!m6502_sameRegs (AOP (left), AOP (result)))
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
	  if(sign)
	    {
	      m6502_emitCmp(m6502_reg_a, 0x80);
	      m6502_rmwWithReg ("ror", m6502_reg_a);
	    }
	  else
	    m6502_rmwWithReg ("lsr", m6502_reg_a);

	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
	  m6502_rmwWithReg ("ror", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  m6502_rmwWithReg ("ror", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
	}
      else
        {
          needloadx = storeRegTempIfUsed (m6502_reg_x);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
	  if(sign)
	    {
	      m6502_emitCmp(m6502_reg_a, 0x80);
	      m6502_rmwWithReg ("ror", m6502_reg_a);
	    }
	  else
	    m6502_rmwWithReg ("lsr", m6502_reg_a);

	  m6502_loadRegFromAop (m6502_reg_x, AOP (left), 2);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_transferRegReg(m6502_reg_x, m6502_reg_a, true);
	  m6502_rmwWithReg ("ror", m6502_reg_a);
	  m6502_loadRegFromAop (m6502_reg_x, AOP (left), 1);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_transferRegReg(m6502_reg_x, m6502_reg_a, true);
	  m6502_rmwWithReg ("ror", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
          m6502_dirtyReg(m6502_reg_x);
        }
      shift-=9;
    }
  shiftRLongInPlace (result, shift, 1, sign, false);

  if(sign)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (result), 2);
      m6502_signExtendReg(m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
    }
  else
    {
      m6502_storeConstToAop (0, AOP (result), 3);
    }

 release:
  m6502_loadOrFreeRegTemp(m6502_reg_x, needloadx);
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * shiftRLong4 - shift right one long from left to result
 *
 * @param left  pointer to the src aop
 * @param result  pointer to the dst aop
 * @param shift  number of shifts (must be >0 and <8)
 *************************************************************************/
static void
shiftRLong4 (operand * left, operand * result, int shift, int sign)
{
  bool needpulla = false;

  wassertl(shift>0, "shiftRLong4 - shift==0");
  wassertl(shift<8, "shiftRLong4 - shift>=8");

  needpulla = pushRegIfUsed (m6502_reg_a);

  if(shift>4)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      if (shift!=7)
	storeRegTemp(m6502_reg_a, true);

      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);

      if (sign)
	{
	  m6502_loadRegFromConst(m6502_reg_a, 0);
	  m6502_emitOp("adc", "#0xff");
	  m6502_emitOp("eor", "#0xff");
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	}
      else
	{
	  m6502_loadRegFromConst(m6502_reg_a, 0);
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	}

      if(shift!=7)
	{
          while(shift!=7)
	    {
	      m6502_emitRegTempOp( "rol", m6502_getLastTempOfs() );
	      m6502_rmwWithAop ("rol", AOP (result), 0);
	      m6502_rmwWithAop ("rol", AOP (result), 1);
	      m6502_rmwWithAop ("rol", AOP (result), 2);
	      m6502_rmwWithAop ("rol", AOP (result), 3);
	      shift++;
	    }
          m6502_loadRegTemp(NULL);
        }
    }
  else
    {
      if(!m6502_sameRegs (AOP (left), AOP (result)))
	{ 
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 3);
          if(sign)
            {
              m6502_emitCmp(m6502_reg_a, 0x80);
              m6502_rmwWithReg ("ror", m6502_reg_a);
	    }
          else
	    m6502_rmwWithReg ("lsr", m6502_reg_a);

	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 3);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 2);
	  m6502_rmwWithReg ("ror", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  m6502_rmwWithReg ("ror", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  m6502_rmwWithReg ("ror", m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
          shiftRLongInPlace (result, shift-1, 0, sign, false);
	  //          m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
	}
      else
	{
	  //  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
          shiftRLongInPlace (result, shift, 0, sign, false);
	  //  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
        }
    }
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * genrsh32 - shift four byte by a known amount != 0
 *************************************************************************/
static void
genrsh32 (operand * result, operand * left, int shCount, int sign)
{
  m6502_emitComment (TRACEGEN, "  %s - shift=%d", __func__, shCount);

  if (shCount >= 24)
    {
      shiftRLong1(left, result, shCount, sign);
    }
  else if (shCount >= 16)
    {
      shiftRLong2(left, result, shCount, sign);
    }
  else if (shCount >= 8)
    {
      shiftRLong3(left, result, shCount, sign);
    }
  else
    {
      shiftRLong4(left, result, shCount, sign);
    }
}

/**************************************************************************
 * genRightShiftLiteral - right shifting by known count
 *************************************************************************/
static void
genRightShiftLiteral (operand * left, operand * result, int shCount, int sign)
{
  bool restore_x = false;
  int size, offset;

  m6502_emitComment (TRACEGEN, __func__);

  //  size = AOP_SIZE (left);
  /* test the LEFT size !!! */
  size = AOP_SIZE (result);
  m6502_emitComment (TRACEGEN|VVDBG, "  %s - result size=%d, left size=%d",
		     __func__, size, AOP_SIZE (left));

  if (shCount == 0)
    {
      m6502_copy (result, left);
    }
  else if (shCount >= (size * 8))
    {
#if 1
      if (sign)
        {
	  bool needpulla = pushRegIfSurv (m6502_reg_a);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), size - 1);
	  m6502_signExtendReg(m6502_reg_a);
	  for(offset=0;offset<size; offset++)
	    m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);

	  pullOrFreeReg (m6502_reg_a, needpulla);
     
        }
      else
        {
	  for(offset=0;offset<size; offset++)
	    m6502_storeConstToAop (0, AOP (result), offset);
        }
#else
      bool needpulla = pushRegIfSurv (m6502_reg_a);
      if (sign)
	{
	  /* get sign in acc.7 */
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), size - 1);
	}
      //      addSign (result, LSB, sign);
      int offset = LSB;
      int size = (AOP_SIZE (result) - offset);
      if (size > 0) {
	if (sign) {
	  m6502_signExtendReg(m6502_reg_a);
	  while (size--)
	    m6502_storeRegToAop (m6502_reg_a, AOP (result), offset++);
	} else
	  while (size--)
	    m6502_storeConstToAop (0, AOP (result), offset++);
      }
      pullOrFreeReg (m6502_reg_a, needpulla);
#endif
    }
  else
    {
      // FIXME: should move this to each genrsh
      if(AOP_TYPE(left)==AOP_SOF || AOP_TYPE(result)==AOP_SOF)
	restore_x=storeRegTempIfSurv(m6502_reg_x);

      switch (size)
	{
	case 1:
	  genrsh8 (result, left, shCount, sign);
	  break;
	case 2:
	  genrsh16 (result, left, shCount, sign);
	  break;
	case 4:
	  genrsh32 (result, left, shCount, sign);
	  break;
	default:
	  emitcode("ERROR", "%s: Invalid operand size %d", __func__, size);
	  break;
	}
      m6502_loadOrFreeRegTemp(m6502_reg_x, restore_x);
    }
}

/**************************************************************************
 * genRightShift - generate code for right shifting
 *************************************************************************/
void
m6502_genRightShift (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int size, offset;
  symbol *loop_label, *skip_label;
  reg_info *countreg = NULL;
  bool restore_a = false;
  bool restore_y = false;
  bool sign;

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (right, ic);
  m6502_aopOp (left, ic);
  m6502_aopOp (result, ic);

  m6502_printIC(ic);

  /* if signed then we do it the hard way preserve the
     sign bit moving it inwards */
  sign = !SPEC_USIGN (getSpec (operandType (left)));

  /* signed & unsigned types are treated the same : i.e. the
     signed is NOT propagated inwards : quoting from the
     ANSI - standard : "for E1 >> E2, is equivalent to division
     by 2**E2 if unsigned or if it has a non-negative value,
     otherwise the result is implementation defined ", MY definition
     is that the sign does not get propagated */

  /* if the shift count is known then do it
     as efficiently as possible */
  if (AOP_TYPE (right) == AOP_LIT &&
      (getSize (operandType (result)) == 1 || getSize (operandType (result)) == 2 || getSize (operandType (result)) == 4))
    {
      int shCount = (int) ulFromVal (AOP (right)->aopu.aop_lit);
      genRightShiftLiteral (left, result, shCount, sign);
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

  if(IS_AOP_A(AOP(right)) && IS_AOP_Y(AOP(left)) && !m6502_reg_x->isDead)
    {
      operand *temp;
      temp = right;
      right = left;
      left = temp;
      storeRegTemp(m6502_reg_a, true);
      m6502_transferRegReg(m6502_reg_y, m6502_reg_a, true);
      m6502_loadRegTemp(m6502_reg_y);
    }

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

  int a_loc = ( src_x | dst_x )? 0 : size-1; // FIXME: should change to msb_in_x ?

  /* find a count register */
  if (m6502_reg_y->isDead && !IS_AOP_WITH_Y (AOP (result)) && !IS_AOP_WITH_Y (AOP (left)))
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
	  storeRegTemp(m6502_reg_y, true);
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
      m6502_loadRegFromAop (countreg, AOP (right), 0);
    }

  if(src_x)
    {
      m6502_emitComment (TRACEGEN, "  %s - src op has x", __func__);
      
      // FIXME: optimize if X is literal (sign is known)
      if(m6502_reg_x->isLitConst)
	{
	  if(m6502_reg_x->litConst<0x80)
	    sign=false;
	}

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
      // FIXME: optimize if X is literal (sign is known)
      if(m6502_reg_x->isLitConst)
	{
	  if(m6502_reg_x->litConst<0x80)
	    sign=false;
	}
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

      m6502_storeRegToAop (m6502_reg_a, AOP(result) , a_loc);

      m6502_emitCmp(countreg, 8);
      m6502_emitBranch ("bcc", skiplbl);
      m6502_safeEmitLabel (looplbl);
      m6502_dirtyAllRegs();

      m6502_loadRegFromAop (m6502_reg_a, AOP (result), 1);
      m6502_storeRegToAop (m6502_reg_a, AOP(result) , 0);
      m6502_loadRegFromAop (m6502_reg_a, AOP (result), 2);
      m6502_storeRegToAop (m6502_reg_a, AOP(result) , 1);
      m6502_loadRegFromAop (m6502_reg_a, AOP (result), 3);
      m6502_storeRegToAop (m6502_reg_a, AOP(result) , 2);

      if(size==8)
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (result), 4);
	  m6502_storeRegToAop (m6502_reg_a, AOP(result) , 3);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (result), 5);
	  m6502_storeRegToAop (m6502_reg_a, AOP(result) , 4);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (result), 6);
	  m6502_storeRegToAop (m6502_reg_a, AOP(result) , 5);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (result), 7);
	  m6502_storeRegToAop (m6502_reg_a, AOP(result) , 6);
	}

      if(sign)
        m6502_signExtendReg(m6502_reg_a);
      else
        m6502_loadRegFromConst (m6502_reg_a, 0);

      m6502_storeRegToAop (m6502_reg_a, AOP(result), a_loc);


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

  if(a_loc==0 && size==2)
    {
      if(sign)
	{
          m6502_emitCmp(m6502_reg_x, 0x80);
	  if(msb_in_x)
	    m6502_emitRegTempOp("ror", m6502_getLastTempOfs() );
	  else
	    m6502_rmwWithAop ("ror", AOP (result), 1);
	}
      else
        {
	  if(msb_in_x)
	    m6502_emitRegTempOp( "lsr", m6502_getLastTempOfs() );
	  else
	    m6502_rmwWithAop ("lsr", AOP (result), 1);
        }
      m6502_rmwWithReg ("ror", m6502_reg_a);
    }
  else
    {
      if(sign)
        {
          m6502_emitCmp(m6502_reg_a, 0x80);
          m6502_rmwWithReg ("ror", m6502_reg_a);
        }
      else
        m6502_rmwWithReg ("lsr", m6502_reg_a);

      for (offset = size - 2; offset >= 0; offset--)
        m6502_rmwWithAop ("ror", AOP (result), offset);
    }

  m6502_rmwWithReg ("dec", countreg);
  m6502_emitBranch ("bne", loop_label);

  if (msb_in_x && countreg!=m6502_reg_x)
    m6502_loadRegTemp(m6502_reg_x);

  m6502_safeEmitLabel (skip_label); // end label

  if (msb_in_x && countreg==m6502_reg_x)
    m6502_loadRegTemp(m6502_reg_x);


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
      m6502_dirtyReg (countreg);
      countreg->isLitConst = 1;
      countreg->litConst = 0;
    }

  if(restore_y)
    m6502_loadRegTemp (m6502_reg_y);
  if(restore_a)
    m6502_loadRegTemp (m6502_reg_a);

 release:
  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (result, NULL);
}

