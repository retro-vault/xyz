/*-------------------------------------------------------------------------
  genminus.c - source file for subtraction code generation for the MOS6502

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

#define OPCODE  "sbc"
#define OPINCDEC "dec"
#define INIT_CARRY()   m6502_emitSetCarry(1)

/**************************************************************************
 * genMinusDec :- does subtraction with decrement if possible
 *************************************************************************/
static bool
genMinusDec (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int icount;
  unsigned int size = AOP_SIZE (result);
  symbol *tlbl = NULL;

  /* will try to generate an increment */
  /* if the right side is not a literal
     we cannot */
  if (AOP_TYPE (right) != AOP_LIT)
    return false;

  icount = (unsigned int) ulFromVal (AOP (right)->aopu.aop_lit);

  m6502_emitComment (TRACEGEN, "  %s - size=%d  icount=%d", __func__, size, icount);

  m6502_emitComment (TRACEGEN|VVDBG, "  %s: icount = %d, m6502_sameRegs=%d",
		     __func__, icount, m6502_sameRegs (AOP (left), AOP (result)));

  if (icount>255 && ((icount&0xff)!=0) )
    return false;

  if (icount>255)
    {
      int bcount = icount>>8;

      if(size==2 && m6502_sameRegs (AOP (left), AOP (result)))
        {
          if (IS_AOP_WITH_X (AOP (result)) && m6502_reg_x->isLitConst)
            {
	      m6502_loadRegFromConst(m6502_reg_x, m6502_reg_x->litConst - bcount);
              return true;
            }
	  else if(IS_AOP_WITH_X (AOP (result)) && m6502_smallAdjustReg(AOP(result)->aopu.aop_reg[1], -bcount))
            {
              return true;
            }
          else if(bcount<3 && m6502_aopCanIncDec(AOP(result)) )
            {
	      while (bcount--)
                m6502_rmwWithAop (OPINCDEC, AOP (result), 1);

	      return true;
            }
        }
      return false;
    }

  if(IS_AOP_XA (AOP (result)) && icount >=0 )
    {
      m6502_loadRegFromAop (m6502_reg_xa, AOP (left), 0);
      if (icount)
	{
	  tlbl = m6502_safeNewiTempLabel (NULL);
	  INIT_CARRY();
	  m6502_accopWithAop (OPCODE, AOP (right), 0);
	  m6502_emitBranch ("bcs", tlbl);
	  m6502_rmwWithReg (OPINCDEC, m6502_reg_x);
	  m6502_safeEmitLabel (tlbl);
	  m6502_dirtyReg(m6502_reg_x);
	}
      return true;
    }

  if (!m6502_sameRegs (AOP (left), AOP (result)))
    {
      if (icount==1 && size==1 )
        {
          reg_info *src_reg = (AOP_TYPE(left)==AOP_REG)? AOP(left)->aopu.aop_reg[0] :NULL;
          reg_info *dst_reg = (AOP_TYPE(result)==AOP_REG)? AOP(result)->aopu.aop_reg[0] :NULL;

	  if(src_reg)
	    {
	      if(dst_reg && dst_reg!=m6502_reg_a)
		{
		  m6502_transferRegReg (src_reg, dst_reg, src_reg->isDead);
		  m6502_rmwWithReg (OPINCDEC, dst_reg);
		  return true;  
		}
	      if(src_reg->isDead /* && src_reg!=m6502_reg_a */ )
		{
		  m6502_rmwWithReg (OPINCDEC, src_reg);
	          m6502_storeRegToAop (src_reg, AOP (result), 0);
		  return true;  
		}
	    }
	}
      return false;
    }

  // m6502_sameRegs

  if (!m6502_aopCanIncDec (AOP (result)))
    return false;

  m6502_emitComment (TRACEGEN|VVDBG, "    %s - sameregs", __func__);

  if (size==1 && AOP(result)->type==AOP_REG)
    {
      // if it's in a 8-bit register try to do small adjust
      if(m6502_smallAdjustReg(AOP(result)->aopu.aop_reg[0], -icount))
	return true;
    }

  if (icount < 0)
    return false;

  if(icount==1 && size==2)
    {
      reg_info *reg = m6502_getFreeByteReg();
      if(reg)
        {
          bool needpullx=false;
          bool needpulla=false;

          if(AOP_TYPE(result)==AOP_SOF || AOP_TYPE(left)==AOP_SOF)
            {
              needpullx=storeRegTempIfSurv(m6502_reg_x);
              needpulla=storeRegTempIfSurv(m6502_reg_a);
              reg=m6502_reg_a;
            }

          tlbl = m6502_safeNewiTempLabel (NULL);
          m6502_loadRegFromAop (reg, AOP (left), 0);
          m6502_emitBranch ("bne", tlbl);
          m6502_rmwWithAop (OPINCDEC, AOP (result), 1);
          m6502_safeEmitLabel (tlbl);
          m6502_rmwWithAop (OPINCDEC, AOP (result), 0);
          m6502_loadOrFreeRegTemp(m6502_reg_a, needpulla);
          m6502_loadOrFreeRegTemp(m6502_reg_x, needpullx);
          return true;
        }
    }

  if (size != 1|| icount>1)
    return false;

  m6502_rmwWithAop (OPINCDEC, AOP (result), 0);

  return true;
}

/**************************************************************************
 * genMinus - generates code for subtraction
 *************************************************************************/
void
m6502_genMinus (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  bool init_carry = true;
  int size, offset;
  bool savea = false;

  sym_link *resulttype = operandType (IC_RESULT (ic));
  unsigned topbytemask = (IS_BITINT (resulttype) && SPEC_USIGN (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;
  bool maskedtopbyte = (topbytemask != 0xff);

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (left, ic);
  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);

  m6502_printIC(ic);

  if (!maskedtopbyte && genMinusDec (ic))
    goto release;

  m6502_emitComment (TRACEGEN|VVDBG, "    %s - Can't %s", __func__, OPINCDEC);

  size = AOP_SIZE (result);
  bool is_right_byte = (AOP_SIZE(right)==1) 
    || ( AOP_TYPE (right) == AOP_LIT
	 && operandLitValue (right) >= 0
	 && operandLitValue (right) <= 255 );


  if ( size==2 && is_right_byte && !maskedtopbyte
       && AOP_TYPE(result) != AOP_SOF
       && m6502_sameRegs(AOP(result),AOP(left)) )
    {
      symbol *skiplabel = m6502_safeNewiTempLabel (NULL);

      m6502_emitComment (TRACEGEN|VVDBG, "    %s: size==2 && one byte", __func__);
      savea = fastSaveAIfSurv ();
      INIT_CARRY();
      m6502_loadRegFromAop (m6502_reg_a, AOP(left), 0);
      m6502_accopWithAop (OPCODE, AOP(right), 0);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      m6502_emitBranch ("bcs", skiplabel);
      m6502_rmwWithAop (OPINCDEC, AOP(result), 1);
      if(IS_AOP_WITH_X(AOP(result)))
	m6502_dirtyReg(m6502_reg_x);

      if(IS_AOP_WITH_Y(AOP(result)))
	m6502_dirtyReg(m6502_reg_y);

      m6502_safeEmitLabel (skiplabel);
      fastRestoreOrFreeA (savea);
      goto release;
    }

  if ( IS_AOP_XA (AOP(right)) && !IS_AOP_XA(AOP(result)) &&
       (AOP_TYPE(result) == AOP_SOF || AOP_TYPE(left) == AOP_SOF) )
    {
      bool restore_a = !m6502_reg_a->isDead;
      bool restore_x = !m6502_reg_x->isDead;
      int a_loc, x_loc;
      storeRegTemp(m6502_reg_a, true);
      a_loc=m6502_getLastTempOfs();
      storeRegTemp(m6502_reg_x, true);
      x_loc=m6502_getLastTempOfs();

      INIT_CARRY();
      m6502_loadRegFromAop (m6502_reg_a, AOP(left), 0);
      m6502_emitRegTempOp(OPCODE, a_loc);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);

      m6502_loadRegFromAop (m6502_reg_a, AOP(left), 1);
      m6502_emitRegTempOp(OPCODE, x_loc);
      if (maskedtopbyte)
	m6502_emitOp ("and", IMMDFMT, topbytemask);

      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);

      if(restore_x)
	m6502_loadRegTemp(m6502_reg_x);
      else
	m6502_loadRegTemp(NULL);
       
      if(restore_a)
	m6502_loadRegTemp(m6502_reg_a);
      else
	m6502_loadRegTemp(NULL);

      goto release;
    }

  if ( IS_AOP_XA (AOP(left)) && !IS_AOP_XA(AOP(result)) &&
       (AOP_TYPE(result) == AOP_SOF || AOP_TYPE(right) == AOP_SOF) )
    {
      bool restore_x = !m6502_reg_x->isDead;

      savea = fastSaveAIfSurv();
      storeRegTemp(m6502_reg_x, true);

      m6502_emitTSX();
      m6502_loadRegFromAop (m6502_reg_a, AOP(left), 0);
      INIT_CARRY();
      m6502_accopWithAop (OPCODE, AOP (right), 0);

      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      m6502_loadRegTempAt(m6502_reg_a, m6502_getLastTempOfs() );
      m6502_accopWithAop (OPCODE, AOP (right), 1);
      if (maskedtopbyte)
	m6502_emitOp ("and", IMMDFMT, topbytemask);

      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);

      if(restore_x)
	m6502_loadRegTemp(m6502_reg_x);
      else
	m6502_loadRegTemp(NULL);

      fastRestoreOrFreeA (savea);
      goto release;
    }

  if (IS_AOP_A (AOP(right)))
    {
      // op - a = neg(a - op) = not(a - op) + 1 = not(a - op - 1)
      savea = fastSaveAIfSurv ();
      m6502_emitSetCarry(0);
      m6502_accopWithAop (OPCODE, AOP(left) , 0);
      m6502_emitOp("eor", "#0xff");
      if (maskedtopbyte)
	m6502_emitOp ("and", IMMDFMT, topbytemask);

      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      fastRestoreOrFreeA (savea);
      goto release;
    }

  if (IS_AOP_WITH_Y (AOP(result)))
    m6502_useReg(m6502_reg_y);

  savea = fastSaveAIfSurv ();

  m6502_emitComment (TRACEGEN|VVDBG, "    %s - general case size=%d", __func__, size);

  for(offset=0; offset<size; offset++)
    {
      if (AOP_TYPE (right) == AOP_REG && AOP (right)->aopu.aop_reg[offset]->rIdx == A_IDX)
	{
	  storeRegTemp (m6502_reg_a, true);
	  m6502_loadRegFromAop (m6502_reg_a, AOP(left), offset);
	  if (init_carry)
	    INIT_CARRY();

	  m6502_emitRegTempOp(OPCODE, m6502_getLastTempOfs() );
 	  m6502_loadRegTemp (NULL);
	}
      else
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP(left), offset);
	  if (init_carry)
	    INIT_CARRY();

	  m6502_accopWithAop (OPCODE, AOP(right), offset);
	}

      if ( (offset==size-1) && maskedtopbyte)
	m6502_emitOp ("and", IMMDFMT, topbytemask);

      if ( offset==0 && IS_AOP_XA (AOP(result)) )
	{
	  m6502_emitComment (TRACEGEN|VVDBG, "  %s - save offset=%d", __func__, offset);
 	  m6502_fastSaveA();

          if(savea)
            emitcode("ERROR", " %s - needpulla && delayedstore == true ", __func__);

	  savea = true;
	}
      else
	{
	  m6502_emitComment (TRACEGEN|VVDBG, "  %s - store offset=%d", __func__, offset);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);
	}

      init_carry = false;
    }

  fastRestoreOrFreeA (savea);

 release:
  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (result, NULL);
}

