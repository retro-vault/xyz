/*-------------------------------------------------------------------------
  genrot.c - source file for rotate code generation for the MOS6502

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

/**************************************************************************
 * genRRC - rotate right with carry
 *************************************************************************/
static void
genRRC (iCode * ic)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int size, offset;
  bool resultInA = false;
  char *shift;

  m6502_emitComment (TRACEGEN, __func__);

  /* rotate right with carry */
  m6502_aopOp (left, ic);
  m6502_aopOp (result, ic);
  m6502_printIC (ic);

  if(IS_AOP_WITH_A(AOP(result))) resultInA=true;
  size = AOP_SIZE (result);
  offset = size - 1;

  shift = "lsr";
  if(IS_AOP_XA(AOP(left)))
    {
      storeRegTempAlways (m6502_reg_x, true);
      m6502_emitRegTempOp ("lsr", m6502_getLastTempOfs() );
      m6502_emitOp ("ror", "a");
      if(IS_AOP_XA(AOP(result)) )
        {
	  storeRegTemp (m6502_reg_a, true);
	  m6502_loadRegFromConst (m6502_reg_a, 0);
	  m6502_emitOp("ror", "a");
	  m6502_emitRegTempOp ("ora", m6502_getLastTempOfs()-1 );
	  m6502_transferRegReg (m6502_reg_a, m6502_reg_x, true);
 	  m6502_loadRegTemp (m6502_reg_a);
	}
      else
	{
	  // optimization if the result is in DIR or EXT
	  m6502_storeRegToAop (m6502_reg_a, AOP(result), 0);
	  m6502_loadRegFromConst (m6502_reg_a, 0);
	  m6502_emitOp ("ror", "a");
	  m6502_emitRegTempOp ("ora", m6502_getLastTempOfs() );
	  m6502_storeRegToAop(m6502_reg_a, AOP(result), 1);
        }
      m6502_loadRegTemp (NULL);
      goto release;
    }
  else if (m6502_sameRegs (AOP (left), AOP (result)))
    {
      while (size--)
        {
          m6502_rmwWithAop (shift, AOP (result), offset--);
          shift = "ror";
        }
    }
  else
    {
      while (size--)
        {
          m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
          m6502_rmwWithReg (shift, m6502_reg_a);
          m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);
          shift = "ror";
          offset--;
        }
    }

  if(resultInA) storeRegTemp(m6502_reg_a, true);

  /* now we need to put the carry into the
     highest order byte of the result */
  offset = AOP_SIZE (result) - 1;
  m6502_loadRegFromConst(m6502_reg_a, 0);
  m6502_emitOp ("ror", "a");
  m6502_accopWithAop ("ora", AOP (result), offset);
  m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);

  if(resultInA) m6502_loadRegTemp(m6502_reg_a);

 release:
  //  pullOrFreeReg (m6502_reg_a, needpula);

  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * genRLC - generate code for rotate left with carry
 *************************************************************************/
static void
genRLC (iCode * ic)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int size, offset;
  char *shift;
  bool resultInA = false;
  bool needpulla = false;

  m6502_emitComment (TRACEGEN, __func__);

  /* rotate right with carry */
  m6502_aopOp (left, ic);
  m6502_aopOp (result, ic);
  m6502_printIC(ic);

  if(IS_AOP_WITH_A(AOP(result))) resultInA=true;
  size = AOP_SIZE (result);
  offset = 0;

  shift = "asl";
  if (!resultInA && m6502_sameRegs (AOP (left), AOP (result)))
    {
      while (size--)
	{
	  m6502_rmwWithAop (shift, AOP (result), offset++);
	  shift = "rol";
	}
    }
  else
    {
      while (size--)
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
	  m6502_rmwWithReg (shift, m6502_reg_a);
	  if(offset==0 && resultInA)
            storeRegTemp (m6502_reg_a, true);
	  else
            m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);

	  shift = "rol";
	  offset++;
	}
    }

  /* now we need to put the carry into the
     lowest order byte of the result */
  needpulla=pushRegIfSurv(m6502_reg_a);
  offset = 0;
  m6502_loadRegFromConst(m6502_reg_a, 0);
  m6502_emitOp ("rol", "a");
  if (resultInA)
    {
      m6502_emitRegTempOp("ora", m6502_getLastTempOfs() );
      m6502_loadRegTemp(NULL);
    }
  else
    {
      m6502_accopWithAop ("ora", AOP (result), offset);
    }
  m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);

  pullOrFreeReg (m6502_reg_a, needpulla);

  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * genRotX - rotate a 16/32 bit value by a known amount
 *************************************************************************/
static void
genRotX(iCode *ic, int shCount)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);
  bool resultInXA = false;
  bool needpulla = false;
  int i, size;

  //m6502_emitComment (TRACEGEN, __func__);
  m6502_aopOp (left, ic);
  m6502_aopOp (result, ic);
  m6502_printIC(ic);

  size = AOP_SIZE (result);

  m6502_emitComment (TRACEGEN, "%s - size=%d shCount=%d",__func__, size, shCount);

  // special case swap16
  if(shCount==8 && size==2)
    {
      if( (IS_AOP_XA(AOP(result)) || IS_AOP_XY(AOP(result)))
	  && AOP_TYPE(result)!=AOP_SOF && AOP_TYPE(left)!=AOP_SOF)
	{
          reg_info *lsb_reg = IS_AOP_XA(AOP(result))?m6502_reg_a:m6502_reg_y;
 
	  if(IS_AOP_WITH_X(AOP(result))&&IS_AOP_WITH_X(AOP(left)))
	    {
	      // reg to reg
	      storeRegTemp(m6502_reg_x, true);
	      m6502_transferRegReg(AOP (left)->aopu.aop_reg[0], m6502_reg_x, true);
	      m6502_loadRegTemp(lsb_reg);
	      goto release;
	    }
          else
	    {
	      m6502_loadRegFromAop (lsb_reg, AOP (left), 1);
	      m6502_loadRegFromAop (m6502_reg_x, AOP (left), 0);
	    }
          goto release;
	}
      else if(IS_AOP_XA(AOP(result)))
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  storeRegTempAlways(m6502_reg_a, true);
	  m6502_dirtyRegTemp (m6502_getLastTempOfs());
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
          m6502_loadRegTemp(m6502_reg_x);
          goto release;
	}
      else if(IS_AOP_XY(AOP(result)))
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
          m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);
          goto release;
	}
    }

  if(IS_AOP_WITH_Y(AOP(result)))
    m6502_useReg(m6502_reg_y);

  if(IS_AOP_XA(AOP(result)))
    resultInXA=true;

  if(!resultInXA)
    needpulla=pushRegIfSurv(m6502_reg_a);

  if(resultInXA)
    {
      if(shCount<8)
	{
	  m6502_emitComment (TRACEGEN|VVDBG, "%s - in A",__func__);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  storeRegTempAlways(m6502_reg_a, true);
	  m6502_dirtyRegTemp (m6502_getLastTempOfs());
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
	}
      else
	{
	  shCount-=8;
	  m6502_emitComment (TRACEGEN|VVDBG, "%s - shCount>=8",__func__);
	  // try reversing
#if 0
	  m6502_loadRegFromAop (m6502_reg_x, AOP (left), 1);
	  storeRegTempAlways(m6502_reg_x, true);
	  m6502_dirtyRegTemp (m6502_getLastTempOfs());
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
#else
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  m6502_loadRegFromAop (m6502_reg_x, AOP (left), 1);
	  storeRegTempAlways(m6502_reg_x, true);
	  m6502_dirtyRegTemp (m6502_getLastTempOfs());
#endif
	}

      for(i=0;i<shCount;i++)
	{
	  m6502_emitCmp(m6502_reg_a, 0x80);
	  m6502_emitRegTempOp("rol", m6502_getLastTempOfs() );
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	}

      m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);
      m6502_loadRegTemp(m6502_reg_a);
    }
  else
    {
      /////////////////////////////////////////////////////////
      // not XA
      /////////////////////////////////////////////////////////
      int msb = size-1;
      int offset;
      int ror = false;
      if((shCount%8)>4)
        {
	  m6502_emitComment (TRACEGEN|VVDBG, "%s - enable ROR",__func__);
          shCount+=8;
          shCount%=(8*size);
          ror=true;
        }

      if(shCount<8)
	{
	  for(offset=0;offset<size-1;offset++)
	    m6502_transferAopAop (AOP(left), offset, AOP (result), offset);

	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), msb);
	}
      else if(shCount<16)
	{
	  shCount-=8;
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), msb-1);
	  for(offset=msb-1;offset>=0;offset--)
	    m6502_transferAopAop (AOP(left), (offset-1+size)%size, AOP (result), offset);
	} 
      else if(shCount<24)
	{
	  shCount-=16;
	  if(!m6502_sameRegs (AOP (left), AOP (result)))
	    {
	      for(offset=0;offset<size-1;offset++)
		m6502_transferAopAop (AOP(left), (offset+2)%size, AOP (result), offset);

	      m6502_loadRegFromAop (m6502_reg_a, AOP (left), (msb+2)%size);
	    }
	  else
	    {
	      //FIXME: only works for 32-bit
	      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	      m6502_transferAopAop (AOP(left), 2, AOP (result), 0);
	      m6502_storeRegToAop (m6502_reg_a, AOP (result), 2);
	      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
	      m6502_transferAopAop (AOP(left), 3, AOP (result), 1);
	    }
	}
      else
	{
	  shCount-=24;
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
	  for(offset=1;offset<size;offset++)
	    m6502_transferAopAop (AOP(left), offset, AOP (result), offset-1);

	  //      for(offset=0;offset<size-1;offset++)
	  //        m6502_transferAopAop (AOP(left), offset, AOP (result), offset);
	  //      m6502_loadRegFromAop (m6502_reg_a, AOP (left), msb);

	}

      if(IS_AOP_XA(AOP(left)))
	m6502_freeReg(m6502_reg_x);

      if(ror)
	{
	  // rotate right
	  shCount=8-shCount;
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), msb);

	  while(shCount--)
	    {
	      m6502_loadRegFromAop (m6502_reg_a, AOP (result), 0);
	      m6502_rmwWithReg ("lsr", m6502_reg_a);

	      for(offset=size-1;offset>=0;offset--)
		m6502_rmwWithAop ("ror", AOP (result), offset);

	    }

	}
      else
	{
	  for(i=0;i<shCount;i++)
	    {
	      m6502_emitCmp(m6502_reg_a, 0x80);
	      for(offset=0;offset<size-1;offset++)
		m6502_rmwWithAop ("rol", AOP (result), offset);

	      m6502_rmwWithReg ("rol", m6502_reg_a);
	    }

	  m6502_storeRegToAop (m6502_reg_a, AOP (result), msb);

	}
    }


 release:
  pullOrFreeReg (m6502_reg_a, needpulla);

  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * genRot8 - rotate a 8-bit value by a known amount
 *************************************************************************/
static void
genRot8(iCode *ic, int shCount)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  bool resultInA = false;
  bool needpulla = false;

  m6502_emitComment (TRACEGEN, __func__);
  m6502_emitComment (TRACEGEN, "%s - size=1 shCount=%d",__func__, shCount);

  m6502_aopOp (left, ic);
  m6502_aopOp (result, ic);
  m6502_printIC(ic);

  if(IS_AOP_WITH_A(AOP(result)))
    resultInA=true;

  if(!resultInA)
    needpulla=pushRegIfSurv(m6502_reg_a);

  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);

  if(shCount>5)
    {
      // right
      shCount=8-shCount;

      while(shCount--)
	{
	  storeRegTempAlways (m6502_reg_a, true);
	  m6502_dirtyRegTemp (m6502_getLastTempOfs());
	  m6502_emitRegTempOp("lsr", m6502_getLastTempOfs() );
	  m6502_rmwWithReg ("ror", m6502_reg_a);
	  m6502_loadRegTemp(NULL);
	}
    }
  else
    {
      if(shCount>=4)
	{
	  shCount-=4;
	  m6502_rmwWithReg ("asl", m6502_reg_a);
	  m6502_emitOp ("adc", "#0x80");
	  m6502_rmwWithReg ("rol", m6502_reg_a);
	  m6502_rmwWithReg ("asl", m6502_reg_a);
	  m6502_emitOp ("adc", "#0x80");
	  m6502_rmwWithReg ("rol", m6502_reg_a);  
	}
      if(shCount>0)
	{
	  // left
	  while(shCount--)
	    {
	      m6502_emitCmp(m6502_reg_a, 0x80);
	      m6502_rmwWithReg ("rol", m6502_reg_a);
	    }
	}
    }
  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
  pullOrFreeReg (m6502_reg_a, needpulla);

  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * genRot - generates code for rotation
 *************************************************************************/
void
m6502_genRot (iCode *ic)
{
  operand *left = IC_LEFT (ic);
  operand *right = IC_RIGHT (ic);
  unsigned int lbits = bitsForType (operandType (left));
  if (IS_OP_LITERAL (right) && lbits==8 )
    genRot8(ic, operandLitValueUll (right) % lbits );
  else if (IS_OP_LITERAL (right) && operandLitValueUll (right) % lbits ==  lbits - 1)
    genRRC (ic);
  else if (IS_OP_LITERAL (right) && operandLitValueUll (right) % lbits == 1)
    genRLC (ic);
  else if (IS_OP_LITERAL (right))
    genRotX(ic, operandLitValueUll (right) % lbits );
  else
    emitcode("ERROR", "%s - Unimplemented rotation (lbits=%d)", __func__, lbits);    
}

