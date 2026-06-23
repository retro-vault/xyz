/*-------------------------------------------------------------------------
  genxor.c - source file for XOR code generation for the MOS6502

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

#define OPCODE "eor"
#define NOP_MASK     0x00

/**************************************************************************
 * genXor - code for Exclusive or
 *************************************************************************/
void
m6502_genXor (iCode * ic, iCode * ifx)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int size, offset = 0;
  bool needpulla = false;
  bool isLit = false;
  unsigned long long lit = 0ull;
  unsigned int bytemask;
  int bitpos = -1;

  m6502_emitComment (TRACEGEN, "%s - ifx:%d", 
		     __func__, ifx?1:0);

  m6502_aopOp (left, ic);
  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);
  m6502_printIC(ic);

  /* force literal on the right and reg on the left */
  if (AOP_TYPE (left) == AOP_LIT || AOP_TYPE (right) == AOP_REG)
    {
      if(!IS_AOP_WITH_A (AOP (left)))
	{
	  operand *tmp = right;
	  right = left;
	  left = tmp;
	}
    }

  size = (AOP_SIZE (left) >= AOP_SIZE (right)) ? AOP_SIZE (left) : AOP_SIZE (right);

  isLit = (AOP_TYPE (right) == AOP_LIT);

  if (isLit)
    {
      lit = ullFromVal (AOP (right)->aopu.aop_lit);
      lit &= m6502_litmask(size);
      bitpos = m6502_isLiteralBit (lit) - 1;
      m6502_emitComment (TRACEGEN|VVDBG, "  %s: lit=%04x bitpos=%d", __func__, lit, bitpos);
    }

  // test for flags only
  if (AOP_TYPE (result) == AOP_CRY)
    {
      if(AOP_TYPE(left)==AOP_REG)
	{
	  m6502_emitComment (TRACEGEN|VVDBG, "  %s: special case reg bit %d", __func__, bitpos);

	  if( size==1 && isLit && lit==NOP_MASK ) 
	    {
	      m6502_emitCmp(AOP (left)->aopu.aop_reg[0], 0);
	      m6502_genIfxJump (ifx, "z");
	      goto release;
	    }
        }

      // test A for flags only
      if (IS_AOP_A (AOP (left)))
	{
	  m6502_emitComment (TRACEGEN|VVDBG, "  %s: test A for flags", __func__);

	  if (m6502_reg_a->isDead)
	    m6502_accopWithAop (OPCODE, AOP (right), 0);
	  else
	    {
	      // no dead register available
	      storeRegTemp(m6502_reg_a, true);
	      m6502_accopWithAop (OPCODE, AOP(right), 0);
	      m6502_loadRegTempNoFlags(m6502_reg_a, true); // preserve flags
	    }
	  m6502_genIfxJump (ifx, "z");
	  goto release;
	}

      // test for flags only (general case)
      symbol *tlbl = m6502_safeNewiTempLabel (NULL);

      needpulla = storeRegTempIfSurv (m6502_reg_a);

      for(offset=0; offset<size; offset++)
	{
          bytemask = (isLit) ? (lit >> (offset * 8)) & 0xff : 0x100;

	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
	  if (bytemask != NOP_MASK)
	    m6502_accopWithAop (OPCODE, AOP (right), offset);

	  if (offset<size-1)
	    m6502_emitBranch ("bne", tlbl);
        }
      // FIXME: check bug1875933.c
      m6502_freeReg (m6502_reg_a);
      m6502_safeEmitLabel (tlbl);

      // TODO: better way to preserve flags?
      if (ifx)
	{
	  m6502_loadRegTempNoFlags (m6502_reg_a, needpulla);
	  m6502_genIfxJump (ifx, "z");
	}
      else
	{
	  if (needpulla)
            m6502_loadRegTemp (NULL);
	}
      goto release;
    }

  size = AOP_SIZE (result);

  if(IS_AOP_Y(AOP(result)))
    m6502_useReg(m6502_reg_y);

  unsigned int bmask0 = (isLit) ? ((lit >> (0 * 8)) & 0xff) : 0x100;
  unsigned int bmask1 = (isLit) ? ((lit >> (1 * 8)) & 0xff) : 0x100;
  bool x_const = (IS_AOP_XA(AOP(left)) || IS_AOP_XY(AOP(left))) && (m6502_reg_x->isLitConst);

  if (x_const && (m6502_reg_x->litConst==0) )
    {
      if(AOP_SIZE (result)>1)
        m6502_transferAopAop(AOP(right), 1, AOP(result), 1);

      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_accopWithAop (OPCODE, AOP (right), 0);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      goto release;
    }

  if (x_const && bmask1!=0x100 )
    {
      if(AOP_SIZE (result)>1)
        m6502_storeConstToAop(m6502_reg_x->litConst^bmask1, AOP(result), 1);

      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_accopWithAop (OPCODE, AOP (right), 0);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      goto release;
    }

  //  if(IS_AOP_XA(AOP(result)) || IS_AOP_XY(AOP(result)))
  if(IS_AOP_XA(AOP(result)))
    {
      m6502_emitComment (TRACEGEN|VVDBG, "  %s: XA", __func__);

      if (IS_AOP_A(AOP(left)))
	m6502_storeConstToAop(0x00, AOP(result), 1);
      else if (bmask1==NOP_MASK)
	m6502_transferAopAop(AOP(left), 1, AOP(result), 1);
      else if(IS_AOP_XA(AOP(left)) && m6502_reg_x->isLitConst && m6502_reg_x->litConst==NOP_MASK)
	m6502_transferAopAop(AOP(right), 1, AOP(result), 1);
      else
	{
	  if(IS_AOP_XA(AOP(left)))
	    {
 	      m6502_fastSaveA();
	      needpulla=true;
	    }
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 1);
	  m6502_accopWithAop (OPCODE, AOP (right), 1);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);          
	}

      {
	if(needpulla)
 	  m6502_fastRestoreA();
	else
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);

	if(bmask0!=NOP_MASK)
	  m6502_accopWithAop (OPCODE, AOP (right), 0);
      }
      goto release;
    }

  if(IS_AOP_Y(AOP(result)))
    m6502_reg_y->isFree=false;

  needpulla = fastSaveAIfSurv();

  // prevent from saving A again
  if(needpulla)
    m6502_reg_a->isDead=true;

  m6502_emitComment (TRACEGEN|VVDBG, "  %s: general path", __func__);

  int incdec;
  incdec = m6502_findRegAop (AOP(left), size-1) ? -1 : 1;
  offset = incdec>0 ? 0 : size-1;

  for( ; offset>=0 && offset<size; offset+=incdec)
    {
      bytemask = (isLit) ? ((lit >> (offset * 8)) & 0xff) : 0x100;

      if ( bytemask==NOP_MASK )
	{
	  m6502_transferAopAop(AOP(left), offset, AOP(result), offset);
	}
      else
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
	  m6502_accopWithAop (OPCODE, AOP (right), offset);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);
          m6502_freeReg(m6502_reg_a);
	}
    }

  fastRestoreOrFreeA(needpulla);

 release:
  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (result, NULL);
}

