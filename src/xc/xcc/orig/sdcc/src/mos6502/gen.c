/*-------------------------------------------------------------------------
  gen.c - source file for code generation for the MOS6502

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

extern int allocInfo;

static bool regalloc_dry_run;
static unsigned int regalloc_dry_run_cost_bytes;
static float regalloc_dry_run_cost_cycles;

struct m6502_state_t _S;

extern int m6502_ptrRegReq;
extern int m6502_nRegs;
extern struct dbuf_s *codeOutBuf;

static bool operandsEqu (operand * op1, operand * op2);
static asmop *newAsmop (short type);
static void aopAdrPrepare (asmop * aop, int loffset);
static const char *aopAdrStr (asmop * aop, int loffset, bool bit16);
static void aopAdrUnprepare (asmop * aop, int loffset);
static void updateiTempRegisterUse (operand * op);
static char * aopName (asmop * aop);
static bool keepTSX();
static void genCmpEQorNE (iCode * ic, iCode * ifx);
// TODO: add compareWithAop()
// TODO: add genCmpZero 

static asmop *m6502_aop_pass[8];
asmop m6502_tsxaop;

const char *IMMDFMT = "#0x%02x";
const char *TEMPFMT = "*(REGTEMP+%d)";
const char *TEMPFMT_IND = "[REGTEMP+%d]";
//static char *TEMPFMT_IY = "[REGTEMP+%d],y";

const char *IDXFMT_X = "0x%x,x";
//static char *TEMPFMT_IX = "[(REGTEMP+%d),x]";
const char *DPTRFMT = "*(DPTR+%d)";
const char *INDFMT_IY = "[%s],y";

const int STACK_TOP = 0x100;


const char m6502_cmp[3][4] = { "cmp", "cpx", "cpy" };

#define RESULTONSTACK(x)                        \
  (IC_RESULT(x) && IC_RESULT(x)->aop &&         \
   IC_RESULT(x)->aop->type == AOP_STK )

#define IS_AOPOFS_A(x,o) (((x)->type == AOP_REG) && ((x)->aopu.aop_reg[o]->mask == M6502MASK_A))
#define IS_AOPOFS_X(x,o) (((x)->type == AOP_REG) && ((x)->aopu.aop_reg[o]->mask == M6502MASK_X))
#define IS_AOPOFS_Y(x,o) (((x)->type == AOP_REG) && ((x)->aopu.aop_reg[o]->mask == M6502MASK_Y))


#define IS_SAME_DPTR_OP(op) (AOP(op) && _S.DPTRAttr[0].aop && _S.DPTRAttr[1].aop \
			     && m6502_sameRegs (AOP(op), _S.DPTRAttr[0].aop))

#define ABS(a) (((a)>0)?a:-a)

/**************************************************************************
 * returns the register containing the AOP or null if not found
 *
 * @param aop pointer to aop
 * @param offset offset of the aop
 * @return reg pointer or NULL if not found
 *************************************************************************/
reg_info*
m6502_findRegAop (asmop * aop, int loffset)
{
  reg_info *ret=NULL;

  if (m6502_reg_a->aop
      && m6502_sameRegs(m6502_reg_a->aop, aop) && (m6502_reg_a->aopofs == loffset))
    {
      ret=m6502_reg_a;
    }
  else if (m6502_reg_x->aop
	   && m6502_sameRegs(m6502_reg_x->aop, aop) && (m6502_reg_x->aopofs == loffset))
    {
      ret=m6502_reg_x;
    }
  else if (m6502_reg_y->aop
	   && m6502_sameRegs(m6502_reg_y->aop, aop) && (m6502_reg_y->aopofs == loffset))
    {
      ret=m6502_reg_y;
    }

  return ret;
}

/**************************************************************************
 * Keeps track of last aop for the register
 *
 * @param reg pointer to the register
 * @param aop pointer to aop
 * @param offset offset of the aop
 *************************************************************************/
void
regTrackAop(reg_info *reg, asmop *aop, int offset)
{
  if(!reg)
    emitcode(";ERROR","  %s : called with NULL reg", __func__ );

  switch(reg->rIdx)
    {
    case A_IDX:
    case X_IDX:
    case Y_IDX:
      reg->aop = aop;
      if(aop)
        {
          reg->aop->op = aop->op;
          reg->aopofs = offset;
          m6502_emitComment (REGOPS|VVDBG, "%s -  reg %s = A+%d", __func__, reg->name, offset);
        }
      else
        m6502_emitComment (REGOPS|VVDBG, "%s -  reg %s = cleared", __func__, reg->name);

      break;
    case XA_IDX:
      regTrackAop(m6502_reg_a, aop, offset);
      regTrackAop(m6502_reg_x, aop, offset+1);
      break;
    case XY_IDX:
      regTrackAop(m6502_reg_y, aop, offset);
      regTrackAop(m6502_reg_x, aop, offset+1);
      break;
    default:
      emitcode ("ERROR", " %s - illegal register %s", __func__, reg->name);
      break;
    }
}

/**************************************************************************
 * Marks registers stale based on an aop
 *
 * @param reg pointer to the register to exclude. All registers if NULL.
 * @param aop pointer to aop
 * @param offset offset of the aop
 *************************************************************************/
void
dirtyRegAop(reg_info *reg, asmop *aop, int offset)
{
  m6502_emitComment (REGOPS|VVDBG, " %s - reg=%s  asmop=%08x off=%d",
		     __func__, reg?reg->name:"NULL", aop, offset);

  if(reg==m6502_reg_xa)
    {
      dirtyRegAop(m6502_reg_a, aop, offset);
      dirtyRegAop(m6502_reg_x, aop, offset+1);
      return;
    }

  if(reg==m6502_reg_xy)
    {
      dirtyRegAop(m6502_reg_y, aop, offset);
      dirtyRegAop(m6502_reg_x, aop, offset+1);
      return;
    }

  if(reg!=m6502_reg_a && m6502_reg_a->aop)
    {
      if(m6502_sameRegs (m6502_reg_a->aop, aop) 
         && (m6502_reg_a->aopofs == offset) )
        {
          m6502_emitComment (REGOPS|VVDBG, "  marking A stale");
          m6502_reg_a->aop = NULL;
        }
    }

  if(reg!=m6502_reg_x && m6502_reg_x->aop)
    {
      if(m6502_sameRegs (m6502_reg_x->aop, aop) 
         && (m6502_reg_x->aopofs == offset) )
        {
          m6502_emitComment (REGOPS|VVDBG, "  marking X stale");
          m6502_reg_x->aop = NULL;
        }
    }

  if(reg!=m6502_reg_y && m6502_reg_y->aop)
    {
      if(m6502_sameRegs (m6502_reg_y->aop, aop) 
         && (m6502_reg_y->aopofs == offset) )
        {
          m6502_emitComment (REGOPS|VVDBG, "  marking Y stale");
          m6502_reg_y->aop = NULL;
        }
    }

  if(_S.DPTRAttr[0].aop && m6502_sameRegs(_S.DPTRAttr[0].aop, aop) && _S.DPTRAttr[0].aopofs==offset)
    {
      _S.DPTRAttr[0].aop=NULL;
    }

  if(_S.DPTRAttr[1].aop && m6502_sameRegs(_S.DPTRAttr[1].aop, aop) && _S.DPTRAttr[1].aopofs==offset)
    {
      _S.DPTRAttr[1].aop=NULL;
    }

}

/**************************************************************************
 * Returns a new temp label symbol
 *
 * @param a ????
 * @return label symbol
 *************************************************************************/
symbol *
m6502_safeNewiTempLabel(const char * a)
{
  if(regalloc_dry_run)
    return NULL;
  else
    return newiTempLabel(a);
}

/**************************************************************************
 * Emit the label in the assembly
 *
 * @param a  pointer to the label symbol
 *************************************************************************/
void
m6502_safeEmitLabel(symbol * a)
{ 
  if(!regalloc_dry_run)
    {
      if (a)
        emitLabel(a);
      else
        emitcode(";ERROR","  %s : called with NULL symbol", __func__ );
    }
  _S.lastflag=-1;
  _S.carryValid=0;
}

/**************************************************************************
 * Returns the number for the label
 *
 * @param a  pointer to the label symbol
 * @return label number
 *************************************************************************/
int
m6502_safeLabelNum(symbol * a)
{
  if(regalloc_dry_run)
    return 0;
 
  if(a)
    return labelKey2num(a->key);

  emitcode("ERROR","  %s : called with NULL symbol", __func__ );
  return 0;
}

/**************************************************************************
 * Returns the last regtemp location
 *
 * @return last temp offset
 *************************************************************************/
int
m6502_getLastTempOfs()
{
  return _S.tempOfs-1;
}

/**************************************************************************
 * Returns the cycle count for the instruction
 *
 * @param opcode  pointer to opcode entry in the opcode table
 * @param arg string constant with the opcode argument
 * @return minimum number of cycles for the instruction
 *************************************************************************/
int
m6502_opcodeCycles(const m6502opcodedata *opcode, const char *arg)
{
  int lastpos;
  
  lastpos=(*arg)?strlen(arg)-1:0;
  
  switch (opcode->type)
    {
    case M6502OP_INH: /* Inherent addressing mode */
    case M6502OP_IDD:
    case M6502OP_IDI:
    case M6502OP_BR:  /* Branch (1 byte signed offset) */
      if(opcode->name[0]=='r'&&opcode->name[1]=='t') // rti and rts
        return 6;
      return 2;
    case M6502OP_SPH:
      return 3;
    case M6502OP_SPL:
      return 4;
    case M6502OP_BBR:  /* Branch on bit (1 byte signed offset) */
      return 3;
    case M6502OP_RMW: /* read/modify/write instructions */
      if (!strcmp(arg, "a"))  /* accumulator */
        return 2;
      if (arg[0] == '*') /* Zero page */
        return 5;
      if(lastpos>2 && arg[lastpos-1]!=',' && arg[lastpos]=='x' )
        return 7;
      return 6;  /* absolute */
    
    case M6502OP_REG: /* standard instruction */
    case M6502OP_CMP:
    case M6502OP_LD:
      if (arg[0] == '#') /* Immediate addressing mode */
        return 2;
      if (arg[0] == '*')
        { /* Zero page */
          if(arg[lastpos]=='x' || arg[lastpos]=='y')
            return 4;
          return 3;
        }
      if (arg[0] == '[')
        { /* indirect */
          if(arg[lastpos]==']')
            return 6;
          return 5;
        }
      return 4; /* Otherwise, must be absolute addressing mode */
    
    case M6502OP_ST:
      if (arg[0] == '*')
        { /* Zero page */
          if(arg[lastpos]=='x' || arg[lastpos]=='y')
            return 4;
          return 3;
        }
      if (arg[0] == '[')  /* indirect */
        return 6;
      if(arg[lastpos]=='x' || arg[lastpos]=='y')
        return 5;
      return 4;
    
    case M6502OP_JMP:
      if(opcode->name[1]=='s')
        return 6;
      if(arg[0]=='[')
        return 5;
      return 3;
    default:
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "unknown instruction type in m6502_opcodeSize");
      return 3;
    }
}

/**************************************************************************
 * Emits comments and debug messages
 *
 * @param level  bitfield based on enum debug_messages
 * @param fmt string constant or printf style format string
 *************************************************************************/
void
m6502_emitComment (unsigned int level, const char *fmt, ...)
{
  bool print=false;
  va_list ap;

  va_start (ap, fmt);

  if ( level&DBG_MSG )
    {
      if(!(level&VVDBG))
        print=true;
      else if(DBG_MSG&VVDBG)
        print=true;
    }

  if (level==VASM && options.verboseAsm)
    print=true;

  if (level==ALWAYS)
    print=true;

  if(regalloc_dry_run)
    print=false;  

  if(print)
    va_emitcode (";", fmt, ap);

  va_end (ap);
}

/**************************************************************************
 * Returns register state as a string
 *
 * @return
 *************************************************************************/
const char *
m6502_regInfoStr()
{
  static char outstr[40];
  char regstring[3][10];

  if(m6502_reg_a->aop == &m6502_tsxaop) snprintf(regstring[0],10,"A:%c%c:S%+-3d",
						 (m6502_reg_a->isFree)?'-':'U',
						 (m6502_reg_a->isDead)?'-':'L',
						 m6502_reg_a->stackOffset);
  else if(m6502_reg_a->isLitConst) snprintf(regstring[0],10,"A:%c%c:#%02X ",
					    (m6502_reg_a->isFree)?'-':'U',
					    (m6502_reg_a->isDead)?'-':'L',
					    m6502_reg_a->litConst&0xff );
  else if(m6502_reg_a->aop) snprintf(regstring[0],10,"A:%c%c:A%+-3d",
				     (m6502_reg_a->isFree)?'-':'U',
				     (m6502_reg_a->isDead)?'-':'L',
				     m6502_reg_a->aopofs );
  else snprintf(regstring[0],10,"A:%c%c:??? ",
                (m6502_reg_a->isFree)?'-':'U',
                (m6502_reg_a->isDead)?'-':'L');
  
  if(m6502_reg_x->aop == &m6502_tsxaop) snprintf(regstring[1],10,"X:%c%c:S%+-3d",
						 (m6502_reg_x->isFree)?'-':'U',
						 (m6502_reg_x->isDead)?'-':'L',
						 m6502_reg_x->stackOffset);
  else if(m6502_reg_x->isLitConst) snprintf(regstring[1],10,"X:%c%c:#%02X ",
					    (m6502_reg_x->isFree)?'-':'U',
					    (m6502_reg_x->isDead)?'-':'L',
					    m6502_reg_x->litConst&0xff  );
  else if(m6502_reg_x->aop) snprintf(regstring[1],10,"X:%c%c:A%+-3d",
				     (m6502_reg_x->isFree)?'-':'U',
				     (m6502_reg_x->isDead)?'-':'L',
				     m6502_reg_x->aopofs );
  else snprintf(regstring[1],10,"X:%c%c:??? ",
                (m6502_reg_x->isFree)?'-':'U',
                (m6502_reg_x->isDead)?'-':'L');
  
  if(m6502_reg_y->aop == &m6502_tsxaop) snprintf(regstring[2],10,"Y:%c%c:S%+-3d",
						 (m6502_reg_y->isFree)?'-':'U',
						 (m6502_reg_y->isDead)?'-':'L',
						 m6502_reg_y->stackOffset);
  else if(m6502_reg_y->isLitConst) snprintf(regstring[2],10,"Y:%c%c:#%02X ",
					    (m6502_reg_y->isFree)?'-':'U',
					    (m6502_reg_y->isDead)?'-':'L',
					    m6502_reg_y->litConst&0xff );
  else if(m6502_reg_y->aop) snprintf(regstring[2],10,"Y:%c%c:A%+-3d",
				     (m6502_reg_y->isFree)?'-':'U',
				     (m6502_reg_y->isDead)?'-':'L',
				     m6502_reg_y->aopofs );
  else snprintf(regstring[2],10,"Y:%c%c:??? ",
                (m6502_reg_y->isFree)?'-':'U',
                (m6502_reg_y->isDead)?'-':'L');
  
  const char *flagreg = (_S.lastflag>=0)?m6502_regWithIdx(_S.lastflag)->name:"?";
  
  char carry = (_S.carryValid)?((_S.carry)?'1':'0'):'?';
  
  snprintf(outstr, 40, "%s %s %s F:%s C:%c",
           regstring[0], regstring[1], regstring[2], flagreg, carry );

  return outstr;
}

/**************************************************************************
 * Returns operand information in the passed string
 *
 * @return
 *************************************************************************/
char *
opInfo(char str[64], operand *op)
{
  int size = 0;
  char *type = "";
  
  if(op)
    {
      if(AOP(op))
        size=AOP_SIZE(op);
      if(AOP(op))
        type=aopName(AOP(op));
    }

  if(op==0)
    {
      snprintf(str, 64, "---");
    }
  else if(IS_SYMOP(op))
    {
      if (snprintf(str, 64, "SYM:%s(%s:%d)", op->svt.symOperand->rname, type, size) >= 64)
        {
	  str[63] = 0; // ridiculous workaround to silence GCC warning ‘%s’ directive output may be truncated
	}
    }
  else if(IS_VALOP(op))
    {
      snprintf(str, 64, "VAL(%s:%d)", type, size);
    }
  else if(IS_TYPOP(op))
    {
      snprintf(str, 64, "TYP");
    }
  else
    {
      snprintf(str, 64, "???");
    }

  return str;
}

/**************************************************************************
 * Prints iCode debug information
 *
 * @return
 *************************************************************************/
void
m6502_printIC(iCode *ic)
{
  operand *left, *right, *result;
  char tmpstr[3][64];

  left = IC_LEFT (ic);
  right = IC_RIGHT (ic);
  result = IC_RESULT (ic);

  opInfo(tmpstr[0], result);
  opInfo(tmpstr[1], left);
  opInfo(tmpstr[2], right);

  m6502_emitComment (TRACEGEN|VVDBG, "  [%s] = [%s] %s [%s]", tmpstr[0], 
		     tmpstr[1], getTableEntry (ic->op)->printName, tmpstr[2]);
}

/**************************************************************************
 * m6502_emitOp - emits opcopdes, updates cost and register state
 *
 * @param inst string containing the opcode
 * @param fmt string operands or printf style format string
 *************************************************************************/
void
m6502_emitOp (const char *inst, const char *fmt, ...)
{
  static char verboseFmt[512];
  va_list ap;
  int isize = 0;
  float cycles = 0;
  float probability=1;

  const m6502opcodedata *opcode = m6502_getOpcodeData(inst);

  if(fmt==0) werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "NULL fmt in m6502_emitOp");


  if(opcode)
    {
      isize = m6502_opcodeSize(opcode, fmt);
      cycles = m6502_opcodeCycles(opcode, fmt);
      reg_info *dst_reg = (opcode->dest>=0)?m6502_regWithIdx(opcode->dest):NULL;

      if(opcode->flags&0x82) // zero and negative flags
        _S.lastflag=opcode->dest;

      if(opcode->flags&0x01)
        {
          // carry flag
          _S.carryValid=0;
        }

      // mark the destination register dirty as necessary
      // transfers are handled in the instruction generator
      switch (opcode->type)
        {
        case M6502OP_LD:
          if(fmt[0]=='#' && isdigit(fmt[1]))
            break;

          m6502_dirtyReg(dst_reg);
          break;
        case M6502OP_REG: // target is accumulator
#if 1
          if(dst_reg && dst_reg->isLitConst &&
             fmt[0]=='#' && isdigit(fmt[1]))
            {
              unsigned char b=strtol(&fmt[1],NULL,0);
              if(!strcmp(inst,"and"))
                {
                  dst_reg->litConst&=b;
                  break;
                }
              if(!strcmp(inst,"ora"))
                {
                  dst_reg->litConst|=b;
                  break;
                }
              if(!strcmp(inst,"eor"))
                {
                  dst_reg->litConst^=b;
                  break;
                }
            }
#endif
          if(dst_reg)
            m6502_dirtyReg (dst_reg);
          break;
        case M6502OP_CMP:
          break;
        case M6502OP_INH:
          if(!strcmp(inst,"clc"))
            {
              _S.carryValid=1;
              _S.carry=0;
              break;
            }
          if(!strcmp(inst,"sec"))
            {
              _S.carryValid=1;
              _S.carry=1;
              break;
            }
          if(!strcmp(inst,"tsx") || !strcmp(inst,"txs") )
            {
              m6502_dirtyReg (m6502_reg_x);
	      m6502_reg_x->aop = &m6502_tsxaop;
              m6502_reg_x->stackOffset = -_S.stackPushes;
              break;
            }
          break;
        case M6502OP_RMW: // target is accumulator
          if (!strcmp(fmt, "a"))
            {
              m6502_dirtyReg (m6502_reg_a);
              _S.lastflag=A_IDX;
            }
          // FIXME: add 65c02 INC/DEC A literal
          break;
        case M6502OP_SPL: // stack pull
          _S.stackPushes--;
          // FIXME: add stack tracking
          m6502_dirtyReg(m6502_reg_a);
          break;
        case M6502OP_SPH: // stack push
          _S.stackPushes++;
          break;
        case M6502OP_IDD: // index decrement
          if(dst_reg->isLitConst)
            dst_reg->litConst--;
	  else if(dst_reg->aop==&m6502_tsxaop)
            dst_reg->stackOffset--;
          else
            m6502_dirtyReg(dst_reg);
          break;
        case M6502OP_IDI: // index increment
          if(dst_reg->isLitConst)
            dst_reg->litConst++;
	  else if(dst_reg->aop==&m6502_tsxaop)
            dst_reg->stackOffset++;
          else
            m6502_dirtyReg(dst_reg);
          break;
        case M6502OP_BR: // add penalty for taken branches
          // this assumes:
          // 50% not taken (2 cycles)
          // 40% taken with target in the same page (3 cycles)
          // 10% taken with target in a different page (4 cycles)
          cycles += (0.4 * 1) + (0.1 * 2);
          break;
        case M6502OP_ST:
        case M6502OP_JMP:
        case M6502OP_BBR:
          break;
        }
    }
  else
    {
      emitcode("ERROR","Unimplemented opcode %s", inst);
      isize=10;
      //werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "NULL opcode in m6502_emitOp");
    }
  
  regalloc_dry_run_cost_bytes += isize;
  regalloc_dry_run_cost_cycles += cycles * probability;

  va_start (ap, fmt);
  if (options.verboseAsm)
    {
      char dstring[3][64];
      dstring[0][0]=0;
      dstring[1][0]=0;
      dstring[2][0]=0;
    
      if (DBG_MSG&COST)
        {
          snprintf(dstring[0], 64, " sz=%d cl=%f p=%f",
                   isize, cycles, probability);
        }
    
      if (DBG_MSG&REGALLOC)
        {
          snprintf(dstring[1], 64, " %s",
		   m6502_regInfoStr() );
        }
      if (DBG_MSG&TRACE_STACK)
        {
          snprintf(dstring[2], 64, " stkpush=%d",
                   _S.stackPushes );
        }

      // FIXME: figure out how to align the comments in the asm output
      snprintf(verboseFmt, 512, "%s \t;%s%s%s",
               fmt, dstring[0], dstring[1], dstring[2]);
      va_emitcode (inst, verboseFmt, ap);
    }
  else
    {
      va_emitcode (inst, fmt, ap);
    }
  va_end (ap);
}

/**************************************************************************
 * m6502_unimplemented
 *
 *************************************************************************/
void
m6502_unimplemented(const char *msg)
{
#ifndef DEBUG_UNIMPLEMENTED
  regalloc_dry_run_cost_bytes  += 500;
  regalloc_dry_run_cost_cycles += 500;
  m6502_emitComment (ALWAYS, "%s - %s", __func__, msg);
#else
  regalloc_dry_run_cost_bytes  = 0;
  regalloc_dry_run_cost_cycles = 0;
  emitcode("ERROR","%s - %s", __func__, msg);
#endif
}

/**************************************************************************
 * emitSignedBranch
 *
 *************************************************************************/
void
emitSignedBranch (bool gt, bool eq, symbol * tlbl)
{
  symbol *tlbl2 = m6502_safeNewiTempLabel (NULL);
  symbol *tlbl3 = m6502_safeNewiTempLabel (NULL);

  if (eq && !gt)
    m6502_emitOp ("beq", "%05d$", m6502_safeLabelNum (tlbl));

  if (!eq && gt)
    m6502_emitOp ("beq", "%05d$", m6502_safeLabelNum (tlbl2));

  m6502_emitOp (gt ? "bvs" : "bvc", "%05d$", m6502_safeLabelNum (tlbl2));
  m6502_emitOp ("bpl", "%05d$", m6502_safeLabelNum (tlbl));
  m6502_emitOp ("bmi", "%05d$", m6502_safeLabelNum (tlbl3));
  m6502_safeEmitLabel (tlbl2);
  m6502_emitOp ("bmi", "%05d$", m6502_safeLabelNum (tlbl));
  m6502_safeEmitLabel (tlbl3);
}

/**************************************************************************
 * emitUnsignedBranch
 *
 *************************************************************************/
static void
emitUnsignedBranch (bool gt, bool eq, symbol * tlbl)
{
  symbol *tlbl2 = m6502_safeNewiTempLabel (NULL);
  
  if (eq && !gt)
    m6502_emitOp ("beq", "%05d$", m6502_safeLabelNum (tlbl));
  if (!eq && gt)
    m6502_emitOp ("beq", "%05d$", m6502_safeLabelNum (tlbl2));

  m6502_emitOp (gt ? "bcs" : "bcc", "%05d$", m6502_safeLabelNum (tlbl));
  m6502_safeEmitLabel (tlbl2);
}

/**************************************************************************
 * m6502_emitBranch
 *
 *************************************************************************/
void
m6502_emitBranch (const char *branchop, symbol * tlbl)
{
  if (!strcmp("bls", branchop))
    {
      emitUnsignedBranch(0, 1, tlbl);
    }
  else if (!strcmp("bhi", branchop))
    {
      emitUnsignedBranch(1, 0, tlbl);
    }
  else if (!strcmp("blt", branchop))
    {
      emitSignedBranch(0, 0, tlbl);
    }
  else if (!strcmp("bgt", branchop))
    {
      emitSignedBranch(1, 0, tlbl);
    }
  else if (!strcmp("ble", branchop))
    {
      emitSignedBranch(0, 1, tlbl);
    }
  else if (!strcmp("bge", branchop))
    {
      emitSignedBranch(1, 1, tlbl);
    }
  else
    {
      if (!IS_MOS65C02 && !strcmp(branchop, "bra"))
        branchop = "jmp";
      m6502_emitOp (branchop, "%05d$", m6502_safeLabelNum (tlbl));
    }
}

/**************************************************************************
 * m6502_emitSetCarry - emit CLC/SEC if necessary
 *
 * @param c carry value to set
 *************************************************************************/
void
m6502_emitSetCarry(int c)
{
  if(_S.carryValid && _S.carry==c)
    return;
  if(c)
    m6502_emitOp("sec", "");
  else
    m6502_emitOp("clc", "");
}

/**************************************************************************
 * m6502_emitCmp - emit CMP/CPX/CPY with immediate if necessary
 *
 * @param reg pointer to register
 * @param val immediate to compare with
 * @return true if the instruction was necessary 
 *************************************************************************/
bool
m6502_emitCmp (reg_info *reg, unsigned char v)
{
  if(!reg)
    emitcode ("ERROR", "  %s - reg is NULL", __func__);

  if(v==0 && _S.lastflag==reg->rIdx)
    return false;

  m6502_emitOp(m6502_cmp[reg->rIdx], "#0x%02x", v);

  if(v==0)
    {
      _S.lastflag=reg->rIdx;
      _S.carryValid=1;
      _S.carry=1;
    }

  return true;
}

/**************************************************************************
 * Adjust register by n bytes if possible.
 *
 * @param reg pointer to the register to adjust
 * @param n amount of adjustment (can be positive or negative)
 * @return return true if the ajdust was performed
 *************************************************************************/
bool
m6502_smallAdjustReg (reg_info *reg, int n)
{
  m6502_emitComment (REGOPS, __func__ );

  if(n==0)
    return true;

  if( (reg!=m6502_reg_x) && (reg!=m6502_reg_y) && !IS_MOS65C02)
    return false;
  
  if (n <= -4 || n >= 4)
    {
      return false;
    }

  while (n < 0)
    {
      m6502_rmwWithReg ("dec", reg); /* 1 byte,  2 cycles */
      n++;
    }
  while (n > 0)
    {
      m6502_rmwWithReg ("inc", reg); /* 1 byte,  2 cycles */
      n--;
    }
  return true;
}

/**************************************************************************
 * Associate the current code location with a debugger symbol
 *************************************************************************/
void
m6502_emitDebuggerSymbol (const char *debugSym)
{
  genLine.lineElement.isDebug = 1;
  emitcode ("", "%s ==.", debugSym);
  genLine.lineElement.isDebug = 0;
}

/**************************************************************************
 * Transfer from register(s) sreg to register(s) dreg.
 * If freesrc is true, sreg is marked free and available for reuse.
 * sreg and dreg must be of equal size
 *
 * @param sreg pointer to the source register
 * @param dreg pointer to the destination register
 * @param freesrc free the source register if true
 *************************************************************************/
void
m6502_transferRegReg (reg_info *sreg, reg_info *dreg, bool freesrc)
{
  int srcidx;
  int dstidx;
  char error = 0;
  
  /* Nothing to do if no destination. */
  if (!dreg)
    return;
  
  /* But it's definitely an error if there's no source. */
  if (!sreg)
    {
      //     emitcode("ERROR","%s: src reg is null", __func__);
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "NULL sreg in m6502_transferRegReg");
      return;
    }

  m6502_emitComment (REGOPS, "  %s(%s,%s)", __func__, sreg->name, dreg->name);
  m6502_emitComment (REGOPS, "  %s %s", __func__, m6502_regInfoStr() );

  srcidx = sreg->rIdx;
  dstidx = dreg->rIdx;

  if (srcidx == dstidx)
    {
      m6502_emitComment (REGOPS|VVDBG, "  %s: sameregs", __func__);
      m6502_useReg (dreg);
      return;
    }

  // TODO: make sure regs are killed if clobbered
  switch (dstidx)
    {
    case A_IDX:
      switch (srcidx) {
      case Y_IDX:            /* Y to A */
        m6502_emitOp ("tya", "");
        break;
      case X_IDX:            /* X to A */
        m6502_emitOp ("txa", "");
        break;
      default:
        error = 1;
      }
      break;
    case X_IDX:
      switch (srcidx)
        {
        case A_IDX:            /* A to X */
          m6502_emitOp ("tax", "");
          break;
        case Y_IDX:            /* Y to X */
          if(m6502_reg_y->isLitConst) {
            m6502_loadRegFromConst (m6502_reg_x, m6502_reg_y->litConst);
          } else if(m6502_reg_a->isFree) {
            m6502_transferRegReg(m6502_reg_y, m6502_reg_a, freesrc);
            m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);
          } else {
            if (IS_MOS65C02) {
              m6502_emitOp ("phy", "");
              m6502_emitOp ("plx", "");
            } else {
              storeRegTemp (m6502_reg_y, false);
	      m6502_loadRegTemp (m6502_reg_x);
            }
          }
          break;
        default:
          error = 1;
        }
      break;
    case Y_IDX:
      switch (srcidx)
        {
        case A_IDX:            /* A to Y */
          m6502_emitOp ("tay", "");
          break;
        case X_IDX:            /* X to Y */
          if(m6502_reg_x->isLitConst) {
            m6502_loadRegFromConst (m6502_reg_y, m6502_reg_x->litConst);
          }
          else if(m6502_reg_a->isFree)
            {
              m6502_transferRegReg(m6502_reg_x, m6502_reg_a, freesrc);
              m6502_transferRegReg(m6502_reg_a, m6502_reg_y, true);
            }
          else
            {
              if (IS_MOS65C02)
                {
                  m6502_emitOp ("phx", "");
                  m6502_emitOp ("ply", "");
                }
              else
                {
                  storeRegTemp (m6502_reg_x, false);
		  m6502_loadRegTemp (m6502_reg_y);
                }
            }
          break;
        default:
          error = 1;
        }
      break;
    case XA_IDX:
      switch (srcidx)
        {
        case XY_IDX:           /* XY to XA */
          m6502_transferRegReg(m6502_reg_y, m6502_reg_a, true);
          break;
        default:
          error = 1;
        }
      break;
    case XY_IDX:
      switch (srcidx)
        {
        case XA_IDX:           /* XA to XY */
          m6502_transferRegReg(m6502_reg_a, m6502_reg_y, true);
          break;
        default:
          error = 1;
        }
      break;
    default:
      error = 1;
    }
  
  if(error)
    emitcode("ERROR", "bad combo in m6502_transferRegReg 0x%02x -> 0x%02x", srcidx, dstidx);

  if (freesrc)
    m6502_freeReg (sreg);

  m6502_dirtyReg (dreg);
  m6502_useReg (dreg);

  if(sreg->isLitConst)
    {
      dreg->isLitConst = sreg->isLitConst;
      dreg->litConst = sreg->litConst;
    }
  else
    {
      regTrackAop(dreg, sreg->aop, sreg->aopofs);
      dreg->stackOffset = sreg->stackOffset;
    }

  m6502_emitComment (REGOPS, "  %s %s", __func__, m6502_regInfoStr() );
}

/**************************************************************************
 * m6502_updateCFA - update the debugger information to reflect the current
 *             canonical frame address relative to the stack pointer
 *************************************************************************/
void
m6502_updateCFA (void)
{
  /* there is no frame unless there is a function */
  if (!currFunc)
    return;
  
  if (options.debug && !regalloc_dry_run)
    debugFile->writeFrameAddress (NULL, m6502_reg_sp, 1 + _S.stackBase + _S.stackPushes);
}

/**************************************************************************
 * Return a string with debugging information about an asmop.
 *************************************************************************/
static char *
aopName (asmop * aop)
{
  static char buffer[276];
  char *buf = buffer;

  if (!aop)
    return "(asmop*)NULL";

  switch (aop->type)
    {
    case AOP_IMMD:
      sprintf (buf, "IMMD(%s)", aop->aopu.aop_immd);
      return buf;
    case AOP_LIT:
      sprintf (buf, "LIT(%s)", aopLiteral (aop->aopu.aop_lit, 0));
      return buf;
    case AOP_DIR:
      sprintf (buf, "DIR(%s)", aop->aopu.aop_dir);
      return buf;
    case AOP_EXT:
      sprintf (buf, "EXT(%s)", aop->aopu.aop_dir);
      return buf;
    case AOP_SOF:
      sprintf (buf, "SOF(%s@%d)", OP_SYMBOL (aop->op)->name, aop->aopu.aop_stk);
      return buf;
    case AOP_REG:
      sprintf (buf, "REG(%s%s%s%s)",
               aop->aopu.aop_reg[3] ? aop->aopu.aop_reg[3]->name : "",
               aop->aopu.aop_reg[2] ? aop->aopu.aop_reg[2]->name : "",
               aop->aopu.aop_reg[1] ? aop->aopu.aop_reg[1]->name : "",
               aop->aopu.aop_reg[0] ? aop->aopu.aop_reg[0]->name : "-");
      return buf;
    case AOP_STK:
      return "STK";
    default:
      sprintf (buf, "?%d", aop->type);
      return buf;
    }

  return "?";
}

/**************************************************************************
 * Load register reg from logical offset loffset of aop.
 * For multi-byte registers, loffset is of the lsb reg.
 *************************************************************************/
void
m6502_loadRegFromAop (reg_info * reg, asmop * aop, int loffset)
{
  int regidx = reg->rIdx;

  m6502_emitComment (REGOPS, "      %s (%s, %s, %d)", __func__,
		     reg->name, aopName (aop), loffset);
  
  if (aop->stacked && aop->stk_aop[loffset])
    {
      m6502_loadRegFromAop (reg, aop->stk_aop[loffset], 0);
      return;
    }

  /* If operand is volatile, we cannot optimize. */
  if (!aop->op || isOperandVolatile (aop->op, false))
    goto forceload;

  /* If this register already has this offset of the operand
     then we need only mark it as in use. */
  if (reg->aop && reg->aop->op && aop->op && operandsEqu (reg->aop->op, aop->op) && (reg->aopofs == loffset))
    {
      m6502_useReg (reg);
      m6502_emitComment (REGOPS, "  already had correct value for %s", reg->name);
      return;
    }

  /* check to see if we can transfer from another register */
  if(reg!=m6502_reg_xa && reg!=m6502_reg_xy)
    {
      reg_info *srcreg=m6502_findRegAop(aop, loffset);
      if(srcreg)
	{
	  m6502_emitComment (REGOPS, "  found correct value for %s in %s", reg->name, srcreg->name);
	  m6502_transferRegReg (srcreg, reg, false);
	  m6502_useReg (reg);
	  return;
	}
    }

 forceload:
  switch (regidx)
    {
    case A_IDX:
    case X_IDX:
    case Y_IDX:
      if (aop->type == AOP_REG)
	{
	  if (loffset < aop->size)
	    m6502_transferRegReg (aop->aopu.aop_reg[loffset], reg, false);
	  else
	    m6502_loadRegFromConst (reg, 0); /* TODO: handle sign extension */
	}
      else if (aop->type == AOP_LIT)
	{
	  m6502_loadRegFromConst (reg, byteOfVal (aop->aopu.aop_lit, loffset));
	}
      else if (aop->type == AOP_SOF && regidx == X_IDX)
	{
	  // TODO: add support for ldy aaaa,x
	  bool needloada = storeRegTempIfUsed(m6502_reg_a); // FIXME: maybe push?
	  m6502_emitComment (TRACEGEN|VVDBG, "  %s - reg:%s ofs:%d",
			     __func__, reg->name, loffset);

	  m6502_loadRegFromAop(m6502_reg_a, aop, loffset);
	  m6502_transferRegReg(m6502_reg_a, reg, false);
	  m6502_loadOrFreeRegTemp(m6502_reg_a,needloada);
	}
      else
	{
	  if(aop->type == AOP_SOF)
	    m6502_emitComment (TRACE_STACK|VVDBG, "      %s: A [%d, %d]", __func__, aop->aopu.aop_stk, loffset);
	  aopAdrPrepare(aop, loffset);
	  const char *l = aopAdrStr (aop, loffset, false);
	  m6502_emitOp (regidx == A_IDX ? "lda" : regidx == X_IDX ? "ldx" : "ldy", l);
	  aopAdrUnprepare(aop, loffset);
	  m6502_dirtyReg (reg);
	  if( !isOperandVolatile (aop->op, false))
	    {
	      regTrackAop(reg, aop, loffset);
	    }
	}
      break;
    case XA_IDX:
      m6502_emitComment (REGOPS, "  %s - XA", __func__);
      if (IS_AOP_XA (aop))
	break;
      else if (IS_AOP_XY (aop))
	m6502_transferRegReg (m6502_reg_xy, m6502_reg_xa, false);
      else if(IS_AOP_X(aop))
	{
	  m6502_transferRegReg (m6502_reg_x, m6502_reg_a, false);
	  m6502_loadRegFromConst (m6502_reg_x, 0);
	}
      else if(aop->type == AOP_SOF)
	{
	  m6502_freeReg(m6502_reg_x);
	  m6502_loadRegFromAop (m6502_reg_a, aop, loffset);
	  if(aop->size == 1)
	    m6502_loadRegFromConst (m6502_reg_x, 0);
	  else
	    {
 	      m6502_fastSaveA();
	      m6502_loadRegFromAop (m6502_reg_a, aop, loffset + 1);
	      m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);
	      m6502_fastRestoreA();
	    }
	}
      else
	{
	  if(aop->size == 1)
	    m6502_loadRegFromConst (m6502_reg_x, 0);
	  else
	    m6502_loadRegFromAop (m6502_reg_x, aop, loffset + 1);

	  m6502_loadRegFromAop (m6502_reg_a, aop, loffset);
	}
      break;
    case XY_IDX:
      if (IS_AOP_XY (aop))
	break;
      else if (IS_AOP_XA (aop))
	m6502_transferRegReg (m6502_reg_xa, m6502_reg_xy, false);
      else if(IS_AOP_X(aop))
	{
	  m6502_transferRegReg (m6502_reg_x, m6502_reg_y, false);
	  m6502_loadRegFromConst (m6502_reg_x, 0);
	}
      else if(aop->type == AOP_SOF)
	{
          bool savea = storeRegTempIfSurv(m6502_reg_a);
	  m6502_freeReg(m6502_reg_x);
	  m6502_loadRegFromAop (m6502_reg_a, aop, loffset);

	  if(aop->size == 1)
            {
	      m6502_loadRegFromConst (m6502_reg_x, 0);
              m6502_transferRegReg(m6502_reg_a, m6502_reg_y, true);
            }
	  else
	    {
	      m6502_transferRegReg(m6502_reg_a, m6502_reg_y, true);
	      m6502_loadRegFromAop (m6502_reg_a, aop, loffset + 1);
	      m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);
	    }
          m6502_loadOrFreeRegTemp(m6502_reg_a, savea);
	}
      else
	{
	  if(aop->size == 1)
	    m6502_loadRegFromConst (m6502_reg_x, 0);
          else
	    m6502_loadRegFromAop (m6502_reg_x, aop, loffset + 1);

	  m6502_loadRegFromAop (m6502_reg_y, aop, loffset);
	}
      break;
    }

  m6502_useReg (reg);
}

/**************************************************************************
 * Find a free index register
 *
 * @return pointer to reg_info or NULL if no index register is available
 *************************************************************************/
reg_info*
m6502_getFreeIdxReg()
{
  // TODO: add reentrant and stack auto
  //if (m6502_reg_y->isFree && !m6502_reg_y->isLitConst)
  //   return m6502_reg_y;
  //  else
  if (m6502_reg_x->isFree && !keepTSX())
    return m6502_reg_x;
  else if (m6502_reg_y->isFree)
    return m6502_reg_y;
  else if(m6502_reg_x->isFree)
    return m6502_reg_x;

  return NULL;
}

/**************************************************************************
 * Find any free 8-bit register
 *
 * @return pointer to reg_info or NULL if no register is available
 *************************************************************************/
reg_info*
m6502_getFreeByteReg()
{
  if (m6502_reg_a->isFree)
    return m6502_reg_a;
  else
    return m6502_getFreeIdxReg();
}

// TODO: move more to this one?
reg_info*
m6502_getDeadByteReg()
{
  if (m6502_reg_a->isDead)
    return m6502_reg_a;
  else if (m6502_reg_y->isDead)
    return m6502_reg_y;
  else if (m6502_reg_x->isDead)
    return m6502_reg_x;
  else
    return NULL;
}

/**************************************************************************
 * m6502_storeRegToAop - Store register reg to logical offset loffset of aop.
 *                 For multi-byte registers, loffset is of the lsb reg.
 *************************************************************************/
void
m6502_storeRegToAop (reg_info *reg, asmop * aop, int loffset)
{
  bool needloada = false;
  bool needloadx = false;
  int regidx = reg->rIdx;

  m6502_emitComment (TRACE_AOP, "      %s (%s, %s, %d), stacked=%d",
		     __func__, reg->name, aopName (aop), loffset, aop->stacked);

  if (aop->type == AOP_DUMMY)
    return;

  if (aop->type == AOP_CRY)     /* This can only happen if IFX was optimized */
    return;                     /* away, so just toss the result */

  if (aop->size == 1 && (regidx==XA_IDX || regidx==XY_IDX))
    {
      if(regidx==XA_IDX)
        m6502_storeRegToAop (m6502_reg_a, aop, loffset);
      else
        m6502_storeRegToAop (m6502_reg_y, aop, loffset);

      return;
    }

  if (aop->type == AOP_REG)
    {
      // handle reg to reg
      switch (regidx)
        {
        case A_IDX:
        case X_IDX:
        case Y_IDX:
          m6502_transferRegReg (reg, aop->aopu.aop_reg[loffset], true);
          break;
        case XA_IDX:
          if (IS_AOP_XY (aop))
            {
              m6502_transferRegReg (reg, m6502_reg_xy, false);
            }
          else
            {
              if(!IS_AOP_XA(aop))
		emitcode("ERROR", "%s: unsupported reg in AOP (XA)", __func__);
            }
          break;
        case XY_IDX:
          if (IS_AOP_XA (aop))
            {
              m6502_transferRegReg (reg, m6502_reg_xa, false);
            }
          else
            {
              if(!IS_AOP_XY(aop))
		emitcode("ERROR", "%s: unsupported reg in AOP (XY)", __func__);
            }
          break;
        }
      return;
    }

  if (aop->type == AOP_DIR || aop->type == AOP_EXT)
    {
      // handle ZP and absolute addresses
      switch (regidx)
        {
        case A_IDX:
          m6502_emitOp ("sta", aopAdrStr (aop, loffset, true));
          break;
        case X_IDX:
          m6502_emitOp ("stx", aopAdrStr (aop, loffset, true));
          break;
        case Y_IDX:
          m6502_emitOp ("sty", aopAdrStr (aop, loffset, true));
          break;
        case XA_IDX:
          m6502_storeRegToAop (m6502_reg_a, aop, loffset);
          m6502_storeRegToAop (m6502_reg_x, aop, loffset+1);
          return;
        case XY_IDX:
          m6502_storeRegToAop (m6502_reg_y, aop, loffset);
          m6502_storeRegToAop (m6502_reg_x, aop, loffset+1);
          return;
        }
    }
  else if (aop->type == AOP_SOF)
    {
      // handle stack
      //    int xofs = STACK_TOP + _S.stackBase - reg->stackOffset + aop->aopu.aop_stk + loffset + 1;

      switch (regidx)
        {
        case A_IDX:
          m6502_emitComment (TRACE_STACK|VVDBG, "      %s: A [%d, %d]",
			     __func__, aop->aopu.aop_stk, loffset);
	  if(m6502_reg_x->aop != &m6502_tsxaop)
            {
              needloadx = storeRegTempIfUsed (m6502_reg_x);
              m6502_emitTSX ();
            }
          m6502_emitOp ("sta", aopAdrStr (aop, loffset, false));
          m6502_freeReg (m6502_reg_a);
          m6502_loadOrFreeRegTemp (m6502_reg_x, needloadx);
          break;
        case X_IDX:
        case Y_IDX:
          // TODO: push if live
          needloada = storeRegTempIfUsed (m6502_reg_a);
          m6502_transferRegReg (reg, m6502_reg_a, false);
          m6502_storeRegToAop (m6502_reg_a, aop, loffset);
          m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
          break;
        case XA_IDX:
          m6502_emitComment (REGOPS, "      %s - XA", __func__);
          // options.stackAuto 
          //        m6502_pushReg(m6502_reg_a, true);
          needloadx = storeRegTempIfUsed (m6502_reg_x);
          storeRegTemp (m6502_reg_a, false);
          m6502_transferRegReg (m6502_reg_x, m6502_reg_a, true);
          m6502_emitTSX();
	  m6502_emitOp ("sta", aopAdrStr (aop, loffset + 1, false));
          //        m6502_pullReg(m6502_reg_a);
          m6502_loadRegTemp(m6502_reg_a);
	  m6502_emitOp ("sta", aopAdrStr (aop, loffset, false));
          m6502_loadOrFreeRegTemp(m6502_reg_x, needloadx);
          break;
        case XY_IDX:
          needloada = storeRegTempIfUsed (m6502_reg_a);
          needloadx = storeRegTempIfUsed (m6502_reg_x);
          m6502_transferRegReg (m6502_reg_x, m6502_reg_a, true);
          m6502_emitTSX();
          m6502_emitOp ("sta", aopAdrStr (aop, loffset + 1 , false));
          m6502_transferRegReg (m6502_reg_y, m6502_reg_a, true);
          m6502_emitOp ("sta", aopAdrStr (aop, loffset, false));
          m6502_loadOrFreeRegTemp (m6502_reg_x, needloadx);
          m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
          break;
        default:
          emitcode("ERROR", "%s: bad reg 0x%02x", __func__, regidx);
        }
    }

  dirtyRegAop(reg, aop, loffset);

  if(!reg->isLitConst)
    {
      //      m6502_emitComment (ALWAYS /*TRACE_AOP|VVDBG*/, " %s - looking for stale reg", __func__);
      //      m6502_emitComment (ALWAYS /*TRACE_AOP|VVDBG*/, " %s - reg_a->aop=%08x aop=%08x aop->op=%08x", 
      //                   __func__, m6502_reg_a->aop, aop, aop->op);
      regTrackAop(reg, aop, loffset);
    }
}

/**************************************************************************
 * m6502_loadRegFromConst - Load register reg from constant c.
 *************************************************************************/
void
m6502_loadRegFromConst (reg_info * reg, int c)
{
  m6502_emitComment (REGOPS, __func__ );

  switch (reg->rIdx) {
  case A_IDX:
    c &= 0xff;
    if (reg->isLitConst && reg->litConst == c)
      break;

    if (m6502_reg_y->isLitConst && m6502_reg_y->litConst == c)
      m6502_transferRegReg (m6502_reg_y, reg, false);
    else if (m6502_reg_x->isLitConst && m6502_reg_x->litConst == c)
      m6502_transferRegReg (m6502_reg_x, reg, false);
    else {
      m6502_emitOp ("lda", IMMDFMT, (unsigned int)c);
    }
    break;
  case X_IDX:
    c &= 0xff;
    if (reg->isLitConst) {
      if (reg->litConst == c)
        break;
      if (((reg->litConst + 1) & 0xff) == c)
        {
          m6502_emitOp ("inx", "");
          break;
        }
      if (((reg->litConst - 1) & 0xff) == c)
        {
          m6502_emitOp ("dex", "");
          break;
        }
    }

    if (m6502_reg_a->isLitConst && m6502_reg_a->litConst == c)
      m6502_transferRegReg (m6502_reg_a, reg, false);
    /*
      TODO does not work for X<->Y
      else if (m6502_reg_y->isLitConst && m6502_reg_y->litConst == c)
      m6502_transferRegReg (m6502_reg_y, reg, false);
    */
    else
      {
        m6502_emitOp ("ldx", IMMDFMT, (unsigned int)c);
      }
    break;
  case Y_IDX:
    c &= 0xff;
    if (reg->isLitConst)
      {
        if (reg->litConst == c)
          break;
        if (((reg->litConst + 1) & 0xff) == c)
          {
            m6502_emitOp ("iny", "");
            break;
          }
        if (((reg->litConst - 1) & 0xff) == c)
          {
            m6502_emitOp ("dey", "");
            break;
          }
      }

    if (m6502_reg_a->isLitConst && m6502_reg_a->litConst == c)
      m6502_transferRegReg (m6502_reg_a, reg, false);
    /*
      TODO does not work for X<->Y
      else if (m6502_reg_x->isLitConst && m6502_reg_x->litConst == c)
      m6502_transferRegReg (m6502_reg_x, reg, false);
    */
    else
      {
        m6502_emitOp ("ldy", IMMDFMT, (unsigned int)c);
      }
    break;
  case XA_IDX:
    c &= 0xffff;
    m6502_loadRegFromConst (m6502_reg_x, c >> 8);
    m6502_loadRegFromConst (m6502_reg_a, c);
    break;
  case XY_IDX:
    c &= 0xffff;
    m6502_loadRegFromConst (m6502_reg_x, c >> 8);
    m6502_loadRegFromConst (m6502_reg_y, c);
    break;
  default:
    emitcode ("ERROR", "bad reg 0x%02x in %s", reg->rIdx, __func__);
    return;
  }

  m6502_dirtyReg (reg);
  reg->isLitConst = 1;
  reg->litConst = c;

  m6502_useReg (reg);
}

/**************************************************************************
 * loadRegFromImm - Load register reg from immediate value c.
 *************************************************************************/
static void
loadRegFromImm (reg_info * reg, char * c)
{
  m6502_emitComment (REGOPS, __func__ );

  if(!c) {
    werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "loadRegFromImm called with a null arg pointer");
  }

  if (*c == '#')
    c++;
  switch (reg->rIdx) {
  case A_IDX:
    m6502_emitOp ("lda", "#%s", c);
    break;
  case X_IDX:
    m6502_emitOp ("ldx", "#%s", c);
    break;
  case Y_IDX:
    m6502_emitOp ("ldy", "#%s", c);
    break;
  case XA_IDX:
    m6502_emitOp ("ldx", "#%s >> 8", c);
    m6502_emitOp ("lda", "#%s", c);
    break;
  case XY_IDX:
    m6502_emitOp ("ldx", "#%s >> 8", c);
    m6502_emitOp ("ldy", "#%s", c);
    break;
  default:
    werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "Bad rIdx in m6502_loadRegFromConst");
    return;
  }
  m6502_dirtyReg (reg);
  m6502_useReg (reg);
}

/**************************************************************************
 * m6502_storeConstToAop - Store constant c to logical offset loffset of
 *                   asmop aop.
 *************************************************************************/
void
m6502_storeConstToAop (int c, asmop * aop, int loffset)
{
  m6502_emitComment (REGOPS, __func__ );

  if (aop->stacked && aop->stk_aop[loffset])
    {
      m6502_storeConstToAop (c, aop->stk_aop[loffset], 0);
      return;
    }

  /* If the value needed is already in A, X or Y just store it */
  if (m6502_reg_a->isLitConst && m6502_reg_a->litConst == c)
    {
      m6502_storeRegToAop (m6502_reg_a, aop, loffset);
      return;
    }
  if (m6502_reg_x->isLitConst && m6502_reg_x->litConst == c)
    {
      m6502_storeRegToAop (m6502_reg_x, aop, loffset);
      return;
    }
  if (m6502_reg_y->isLitConst && m6502_reg_y->litConst == c)
    {
      m6502_storeRegToAop (m6502_reg_y, aop, loffset);
      return;
    }

  switch (aop->type)
    {
    case AOP_REG:
      if (loffset > (aop->size - 1))
        break;
      m6502_loadRegFromConst (aop->aopu.aop_reg[loffset], c);
      break;
    case AOP_DUMMY:
      break;
    case AOP_DIR:
    case AOP_EXT:
      /* stz operates with read-modify-write cycles, so don't use if the */
      /* destination is volatile to avoid the read side-effect. */
      if (c==0 && IS_MOS65C02 && !(aop->op && isOperandVolatile (aop->op, false)))
        {
          m6502_emitOp ("stz", "%s", aopAdrStr (aop, loffset, false));
          break;
        }
    default:
      if(aop->type != AOP_SOF)
        {
          reg_info *reg = NULL;

          // prefer X if literal!=0 && X does not contain tsx offset 
          if(c!=0 && m6502_reg_x->isFree && !keepTSX() )
            reg=m6502_reg_x;
          else if(m6502_reg_y->isFree)
#if 0
	    // prefer X if literal!=0 && X does not contain tsx offset
	    // try not to overwrite literal zero
	    // search for +/-1
	    if(c==0 && loffset==0 && m6502_reg_y->isFree)
	      reg=m6502_reg_y;
	    else if(c==0 && m6502_reg_x->isFree)
	      reg=m6502_reg_x;
	    else if(m6502_reg_y->isFree && m6502_reg_y->isLitConst && (m6502_reg_y->litConst!=0))
	      reg=m6502_reg_y;
	    else if(m6502_reg_x->isFree && !keepTSX() && !m6502_reg_x->isLitConst)
	      reg=m6502_reg_x;
	    else if(m6502_reg_y->isFree && !m6502_reg_y->isLitConst)
	      reg=m6502_reg_y;
	    else if(m6502_reg_x->isFree && !keepTSX() && m6502_reg_x->isLitConst && (m6502_reg_x->litConst!=0))
	      reg=m6502_reg_x;
	    else if(m6502_reg_x->isFree && !keepTSX() && m6502_reg_x->isLitConst && (abs(m6502_reg_x->litConst-c) < 2))
	      reg=m6502_reg_x;
	    else if(m6502_reg_y->isFree && m6502_reg_y->isLitConst && !m6502_reg_a->isFree)
#endif
	      reg=m6502_reg_y;

          if(reg)
            {
              m6502_loadRegFromConst (reg, c);
              m6502_storeRegToAop (reg, aop, loffset);
              m6502_freeReg (reg);
              return;
            }
        }
      bool needpulla = pushRegIfUsed (m6502_reg_a);
      // bool needpulla = storeRegTempIfUsed (m6502_reg_a);
      m6502_loadRegFromConst (m6502_reg_a, c);
      m6502_storeRegToAop (m6502_reg_a, aop, loffset);
      // m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
      pullOrFreeReg (m6502_reg_a, needpulla);
    }
}

/**************************************************************************
 * Store immediate value to asmop
 *
 * @param c  pointer to the immediate value
 * @param aop pointer to the asmop
 * @param loffset asmop offset
 *************************************************************************/
static void
storeImmToAop (char *c, asmop * aop, int loffset)
{
  reg_info *reg = NULL;
  bool savea = false;

  m6502_emitComment (TRACE_AOP, __func__ );

  if (aop->stacked && aop->stk_aop[loffset])
    {
      storeImmToAop (c, aop->stk_aop[loffset], 0);
      return;
    }

  switch (aop->type) {
  case AOP_REG:
    if (loffset > (aop->size - 1))
      break;
    loadRegFromImm (aop->aopu.aop_reg[loffset], c);
    break;
  case AOP_DUMMY:
    break;
  case AOP_DIR:
  case AOP_EXT:
    if (!strcmp (c, "#0x00") && IS_MOS65C02 )
      {
        m6502_emitOp ("stz", "%s", aopAdrStr (aop, loffset, false));
        break;
      }
  default:
      
    if(aop->type!=AOP_SOF)
      reg = m6502_getFreeByteReg();

    if (reg == NULL)
      {
 	savea = fastSaveAIfUsed ();
	reg = m6502_reg_a;
      }
    loadRegFromImm (reg, c);
    m6502_storeRegToAop (reg, aop, loffset);
    m6502_freeReg (reg);
    fastRestoreOrFreeA (savea);

  }
}

/**************************************************************************
 * sign extends the register
 *
 * @param reg  pointer to the register
 *************************************************************************/
void
m6502_signExtendReg(reg_info *reg)
{
  symbol *skip_lbl = m6502_safeNewiTempLabel (NULL);

  if(reg==m6502_reg_a)
    m6502_emitOp ("asl", "a");
  else
    m6502_emitCmp (reg, 0x80);

  m6502_loadRegFromConst (reg, 0);
  m6502_emitBranch ("bcc", skip_lbl);
  m6502_loadRegFromConst (reg, 0xff);
  m6502_safeEmitLabel (skip_lbl);
  m6502_dirtyReg (reg);
}

/**************************************************************************
 * storeRegSignToUpperAop - If isSigned is true, the sign bit of register
 *                          reg is extended to fill logical offsets loffset
 *                          and above of asmop aop. Otherwise, logical
 *                          offsets loffset and above of asmop aop are
 *                          zeroed. reg must be an 8-bit register.
 *************************************************************************/
static void
storeRegSignToUpperAop (reg_info * reg, asmop * aop, int loffset, bool isSigned)
{
  m6502_emitComment (TRACE_AOP, __func__ );

  int size = aop->size;

  if (loffset >= size)
    return;

  if (!isSigned)
    {
      /* Unsigned case */
      while (loffset < size)
        m6502_storeConstToAop (0, aop, loffset++);
    }
  else
    {
      /* Signed case */
      m6502_signExtendReg(reg);

      while (loffset < size)
        m6502_storeRegToAop (reg, aop, loffset++);
      m6502_freeReg (reg);
    }
}

/**************************************************************************
 * m6502_storeRegToFullAop - Store register reg to asmop aop with appropriate
 *                     padding and/or truncation as needed. If isSigned is
 *                     true, sign extension will take place in the padding.
 *************************************************************************/
void
m6502_storeRegToFullAop (reg_info *reg, asmop *aop, bool isSigned)
{
  int regidx = reg->rIdx;
  int size = aop->size;

  m6502_emitComment (TRACE_AOP, __func__ );

  switch (regidx)
    {
    case A_IDX:
    case X_IDX:
    case Y_IDX:
#if 0
      // FIXME: this optimization reduce code size but increase runtime
      // should conditionally add it when optmizing for size
      if ( IS_AOP_XA(aop) && regidx==A_IDX )
        {
          m6502_loadRegFromConst(m6502_reg_x,0);
          if (isSigned)
            {
              symbol *tlbl = m6502_safeNewiTempLabel (NULL);
              m6502_emitCmp(m6502_reg_a, 0x80);
              m6502_emitBranch ("bcc", tlbl);
              m6502_loadRegFromConst (m6502_reg_x, 0xff);
              m6502_safeEmitLabel (tlbl);
              m6502_dirtyReg (m6502_reg_x);
            }
        }
      else
#endif
        {
          m6502_storeRegToAop (reg, aop, 0);
          if (size > 1 && isSigned && aop->type == AOP_REG && aop->aopu.aop_reg[0]->rIdx == A_IDX)
            m6502_pushReg (m6502_reg_a, true);
          storeRegSignToUpperAop (reg, aop, 1, isSigned);
          if (size > 1 && isSigned && aop->type == AOP_REG && aop->aopu.aop_reg[0]->rIdx == A_IDX)
            m6502_pullReg (m6502_reg_a);
        }
      break;
    case XA_IDX:
      if (size == 1)
	{
	  m6502_storeRegToAop (m6502_reg_a, aop, 0);
	}
      else
	{
	  m6502_storeRegToAop (reg, aop, 0);
	  if (size>2)
	    {
	      if (aop->type!=AOP_SOF)
		{
		  storeRegSignToUpperAop (m6502_reg_x, aop, 2, isSigned);
		}
	      else
		{
		  storeRegTemp (m6502_reg_a, true);
		  m6502_loadRegFromAop (m6502_reg_a, aop, 1);
		  storeRegSignToUpperAop (m6502_reg_a, aop, 2, isSigned);
		  m6502_loadRegTemp (m6502_reg_a);
		}
	    }
	}
      break;
    case XY_IDX:
      if (size == 1)
	{
	  m6502_storeRegToAop (m6502_reg_y, aop, 0);
	}
      else
	{
	  m6502_storeRegToAop (reg, aop, 0);
	  storeRegSignToUpperAop (m6502_reg_x, aop, 2, isSigned);
	}
      break;
    default:
      emitcode("ERROR", "bad reg 0x%02x in m6502_storeRegToFullAop()", regidx);
    }
}

/**************************************************************************
 * m6502_transferAopAop - Transfer the value at logical offset srcofs of asmop
 *                  srcaop to logical offset dstofs of asmop dstaop.
 *************************************************************************/
void
m6502_transferAopAop (asmop *srcaop, int srcofs, asmop *dstaop, int dstofs)
{
  bool needpulla = false;
  bool freereg = true;
  reg_info *reg = NULL;

  m6502_emitComment (TRACE_AOP, __func__ );

  if(!srcaop || !dstaop)
    {
      if (!srcaop) emitcode("ERROR", "srcaop is null");
      if (!dstaop) emitcode("ERROR", "dstaop is null");
      return;
    }
  wassert (srcaop && dstaop);

  /* ignore transfers at the same byte, unless its volatile */
  if (srcaop->op && !isOperandVolatile (srcaop->op, false)
      && dstaop->op && !isOperandVolatile (dstaop->op, false)
      && m6502_sameRegs (srcaop, dstaop) && srcofs == dstofs && dstaop->type == srcaop->type)
    return;

  if (srcaop->stacked && srcaop->stk_aop[srcofs])
    {
      m6502_transferAopAop (srcaop->stk_aop[srcofs], 0, dstaop, dstofs);
      return;
    }

  if (dstaop->stacked && dstaop->stk_aop[srcofs])
    {
      m6502_transferAopAop (srcaop, srcofs, dstaop->stk_aop[dstofs], 0);
      return;
    }

  m6502_emitComment (TRACE_AOP|VVDBG, "    %s from (%s, %d, 0x%x)",
		     __func__, aopName (srcaop), srcofs, srcaop->regmask);
  m6502_emitComment (TRACE_AOP|VVDBG, "    %s to (%s, %d, 0x%x)",
		     __func__, aopName (dstaop), dstofs, dstaop->regmask);

  if (dstofs >= dstaop->size)
    return;

  // same registers and offset, no transfer
  if (srcaop->type == AOP_REG && dstaop->type == AOP_REG)
    {
      m6502_emitComment (TRACE_AOP|VVDBG, "  %s: regreg", __func__);
      m6502_transferRegReg(srcaop->aopu.aop_reg[srcofs], dstaop->aopu.aop_reg[dstofs], false);
      return;
    }

#if 0
  // same stack offset, no transfer
  if(srcaop->type == AOP_SOF && dstaop->type == AOP_SOF)
    if( (srcaop->aopu.aop_stk+srcofs) == (dstaop->aopu.aop_stk+dstofs) )
      {
	m6502_emitComment (TRACE_AOP|VVDBG, "    %s:  AOP_SOF same offset", __func__);
	return;
      }
#endif

  if (srcaop->type == AOP_LIT)
    {
      m6502_storeConstToAop (byteOfVal (srcaop->aopu.aop_lit, srcofs), dstaop, dstofs);
      return;
    }

  if (dstaop->type == AOP_REG)
    {
      reg = dstaop->aopu.aop_reg[dstofs];
      freereg = false;
    } 
  else if ((srcaop->type == AOP_REG) && (srcaop->aopu.aop_reg[srcofs]))
    {
      reg = srcaop->aopu.aop_reg[srcofs];
      freereg = false;
    }
  else
    {
      if(srcaop->type != AOP_SOF && dstaop->type != AOP_SOF)
        reg = m6502_getFreeByteReg();

      if (reg == NULL)
        {
          // FIXME: used vs. surv triggers failure on bug 3556 in stack-auto
          // seems to not affect the bug anymore (?)
          needpulla = storeRegTempIfUsed (m6502_reg_a);
          reg = m6502_reg_a;
        }
    }

  m6502_emitComment (TRACE_AOP|VVDBG, "  %s: general case", __func__);

  m6502_loadRegFromAop (reg, srcaop, srcofs);
  m6502_storeRegToAop (reg, dstaop, dstofs);

  if (freereg)
    m6502_loadOrFreeRegTemp (reg, needpulla);
}

#if 0
/**************************************************************************
 * forceStackedAop - Reserve space on the stack for asmop aop; when
 *                   m6502_freeAsmop is called with aop, the stacked data will
 *                   be copied to the original aop location.
 *************************************************************************/
// TODO????
static asmop * forceStackedAop (asmop * aop, bool copyOrig)
{
  reg_info *reg = NULL;
  int offset;
  bool needpula = false;
  asmop *newaop = newAsmop (AOP_DIR);
  memcpy (newaop, aop, sizeof (*newaop));
  newaop->aopu.aop_dir = "REGTEMP";

  m6502_emitComment (TRACE_AOP|VVDBG, "  forcedStackedAop %s", aopName (aop));

  if (copyOrig)
    {
      reg = m6502_getFreeByteReg ();
      if (reg == NULL)
	{
	  reg = m6502_reg_a;
	  storeRegTemp(reg, true);
	  needpula = true;
	}
    }
  for (offset=0; offset<newaop->size; offset++)
    {
      asmop *aopsof = newAsmop (AOP_SOF);
      aopsof->size = 1;
      if (copyOrig)
	{
	  m6502_loadRegFromAop (reg, aop, offset);
	  m6502_pushReg (reg, false);
	}
      else
	{
	  m6502_pushReg (m6502_reg_a, false);
	}
      aopsof->aopu.aop_stk = -_S.stackBase - _S.stackPushes;
      aopsof->op = aop->op;
      newaop->stk_aop[offset] = aopsof;
    }

  if (!reg && copyOrig)
    {
      for (offset = 0; offset < newaop->size; offset++)
	{
	  m6502_transferAopAop (aop, offset, newaop, offset);
	}
    }
  newaop->stacked = 1;
  m6502_loadOrFreeRegTemp(reg, needpulla);
  return newaop;
}
#endif

// TODO: fix these
/**************************************************************************
 * m6502_accopWithAop - Emit accumulator modifying instruction accop with
 *                the byte at logical offset loffset of asmop aop.
 *                Supports: adc, and, cmp, eor, ora, sbc
 *************************************************************************/
void
m6502_accopWithAop (const char *accop, asmop *aop, int loffset)
{
  m6502_emitComment (TRACE_AOP, __func__ );

  if (aop->stacked && aop->stk_aop[loffset])
    {
      m6502_accopWithAop (accop, aop->stk_aop[loffset], 0);
      return;
    }

  if (aop->type == AOP_DUMMY)
    return;

  if (aop->type == AOP_REG)
    {
      if (loffset < aop->size)
        {
	  storeRegTemp (aop->aopu.aop_reg[loffset], true);
	  m6502_emitRegTempOp( accop, m6502_getLastTempOfs() );
	  m6502_loadRegTemp(NULL);
        }
      else
	{
	  m6502_emitOp (accop, "#0x00");
        }
    }
  else if(aop->type==AOP_LIT)
    {
      unsigned char v = (ullFromVal(aop->aopu.aop_lit))>>(loffset*8);

      if(strcmp(accop,"cmp")==0)
	{
	  m6502_emitCmp(m6502_reg_a, v);
	}
      else if (v==0)
	m6502_emitOp (accop, "#0x00");
      else
	m6502_emitOp (accop, IMMDFMT, v);
    }
  else
    {
      aopAdrPrepare(aop, loffset);
      //    m6502_emitOp (accop, aopAdrStr (aop, loffset, false));
      const char *arg = aopAdrStr (aop, loffset, false);
      m6502_emitOp (accop, arg);
      aopAdrUnprepare(aop, loffset);
    }
}

/**************************************************************************
 * m6502_rmwWithReg - Emit read/modify/write instruction rmwop with register reg.
 *              byte at logical offset loffset of asmop aop. Register reg
 *              must be 8-bit.
 *              Supports: dec, inc, lsl, lsr, neg, rol, ror
 *************************************************************************/
void
m6502_rmwWithReg (char *rmwop, reg_info * reg)
{
  if (reg->rIdx == A_IDX)
    {
      if (!strcmp(rmwop, "inc") && !IS_MOS65C02)
        {
          m6502_emitSetCarry(0);
          m6502_emitOp ("adc", "#0x01");
        }
      else if (!strcmp(rmwop, "dec")  && !IS_MOS65C02)
        {
          m6502_emitSetCarry(1);
          m6502_emitOp ("sbc", "#0x01");
        }
      else if (!strcmp(rmwop, "neg"))
        {
          m6502_emitOp ("eor", "#0xff");
          m6502_emitSetCarry (0);
          m6502_emitOp ("adc", "#0x01");
        }
      else
        {
          m6502_emitOp (rmwop, "a");
        }
    }
  else if (reg->rIdx == X_IDX || reg->rIdx == Y_IDX)
    {
      if (!strcmp(rmwop, "inc"))
        {
          if (reg->rIdx == X_IDX)
            m6502_emitOp ("inx", "");
          else
            m6502_emitOp ("iny", "");
        }
      else if (!strcmp(rmwop, "dec"))
        {
          if (reg->rIdx == X_IDX)
            m6502_emitOp ("dex", "");
          else
            m6502_emitOp ("dey", "");
        }
      else
        {
          bool needpulla = pushRegIfUsed (m6502_reg_a);
          m6502_transferRegReg (reg, m6502_reg_a, true);
          m6502_rmwWithReg (rmwop, m6502_reg_a);
          m6502_transferRegReg (m6502_reg_a, reg, true);
          pullOrFreeReg (m6502_reg_a, needpulla);
        }
    }
  else
    {
      emitcode("ERROR", "bad reg in m6502_rmwWithReg()");
    }
}

/**************************************************************************
 * m6502_rmwWithAop - Emit read/modify/write instruction rmwop with the byte at
 *                logical offset loffset of asmop aop.
 *                Supports: bit, dec, inc, lsl, lsr, neg, rol, ror
 *************************************************************************/
void
m6502_rmwWithAop (char *rmwop, asmop * aop, int loffset)
{
  //  bool needpull = false;
  m6502_emitComment (TRACE_AOP, __func__ );

  if (aop->stacked && aop->stk_aop[loffset])
    {
      m6502_rmwWithAop (rmwop, aop->stk_aop[loffset], 0);
      return;
    }

  switch (aop->type)
    {
    case AOP_REG:
      m6502_rmwWithReg (rmwop, aop->aopu.aop_reg[loffset]);
      break;

    case AOP_DIR:
    case AOP_EXT:
      m6502_emitComment (TRACE_AOP, "  m6502_rmwWithAop DIR/EXT");
      m6502_emitOp (rmwop, aopAdrStr(aop, loffset, false));
      dirtyRegAop(NULL, aop, loffset);
      break;

    case AOP_SOF:
      m6502_emitComment (TRACE_AOP, "  m6502_rmwWithAop AOP_SOF");
      // FIXME: figure out if asr handling should be here
      // or if asr should be generated at all by the codegen
      m6502_emitOp (rmwop, aopAdrStr (aop, loffset, false));
      dirtyRegAop(NULL, aop, loffset);
      break;

    default:
      break;
    }
}

/**************************************************************************
 * load reg from DPTR at offset dofs
 *************************************************************************/
static void
loadRegFromDPTR(reg_info *reg, int dofs)
{
  int regidx=reg->rIdx;

  if(_S.DPTRAttr[dofs].isLiteral)
    {

      if(reg->isLitConst
	 && reg->litConst == _S.DPTRAttr[dofs].literalValue )
	m6502_emitComment (TRACEGEN, " %s: DPTR[%d] has same literal %02x",
			   __func__, dofs, reg->litConst);
      else
        m6502_loadRegFromConst(reg, _S.DPTRAttr[dofs].literalValue);

      return;
    }

  if ( reg->aop && _S.DPTRAttr[dofs].aop && m6502_sameRegs (reg->aop, _S.DPTRAttr[dofs].aop) 
       && (reg->aopofs == dofs) )
    {
      m6502_emitComment (TRACEGEN, " %s: register already has result", __func__);
      return;
    }

  switch(regidx)
    {
    case A_IDX:
      m6502_emitOp ("lda", DPTRFMT, dofs);
      break;
    case X_IDX:
      m6502_emitOp ("ldx", DPTRFMT, dofs);
      break;
    case Y_IDX:
      m6502_emitOp ("ldy", DPTRFMT, dofs);
      break;
    default:
      emitcode("ERROR","  %s: illegal register index %d", __func__, regidx);
      return;
    }

  reg->isLitConst=_S.DPTRAttr[dofs].isLiteral;
  reg->litConst=_S.DPTRAttr[dofs].literalValue;
  reg->aop=_S.DPTRAttr[dofs].aop;
  reg->aopofs=_S.DPTRAttr[dofs].aopofs;
}

/**************************************************************************
 * stores reg in DPTR at offset dofs
 *************************************************************************/
static void
storeRegToDPTR(reg_info *reg, int dofs)
{
  int regidx=reg->rIdx;

  if(reg->isLitConst && _S.DPTRAttr[dofs].isLiteral
     && reg->litConst == _S.DPTRAttr[dofs].literalValue )
    {
      m6502_emitComment (TRACEGEN, " %s: DPTR[%d] has same literal %02x",
			 __func__, dofs, reg->litConst);
      m6502_freeReg(reg);
      return;
    }

  if ( reg->aop && _S.DPTRAttr[dofs].aop && m6502_sameRegs (reg->aop, _S.DPTRAttr[dofs].aop) 
       && (reg->aopofs == dofs) )
    {
      m6502_emitComment (TRACEGEN, " %s: DPTR already has result", __func__);
      return;
    }

  switch(regidx)
    {
    case A_IDX:
      m6502_emitOp ("sta", DPTRFMT, dofs);
      break;
    case X_IDX:
      m6502_emitOp ("stx", DPTRFMT, dofs);
      break;
    case Y_IDX:
      m6502_emitOp ("sty", DPTRFMT, dofs);
      break;
    default:
      emitcode("ERROR","  %s: illegal register index %d", __func__, regidx);
      return;
    }

  _S.DPTRAttr[dofs].isLiteral=reg->isLitConst;
  _S.DPTRAttr[dofs].literalValue=reg->litConst;
  _S.DPTRAttr[dofs].aop=reg->aop;
  _S.DPTRAttr[dofs].aopofs=reg->aopofs;

  m6502_freeReg(reg);
}

/**************************************************************************
 * sets up DPTR for a indexed operation
 * clobbers A if savea==false and clobbers Y if savea==true
 *************************************************************************/
static int
setupDPTR(operand *op, int offset, char * rematOfs, bool savea)
{
  m6502_emitComment (TRACEGEN, "  %s - off=%d remat=%s savea=%d", __func__, offset, rematOfs, savea?1:0);

  reg_info *savereg = NULL;

  /* The rematerialized offset may have a "#" prefix; skip over it */
  if (rematOfs && rematOfs[0] == '#')
    rematOfs++;
  if (rematOfs && !rematOfs[0])
    rematOfs = NULL;

  /* force offset to signed 16-bit range */
  offset &= 0xffff;
  if (offset & 0x8000)
    offset = 0x10000 - offset;
  //    offset = offset - 0x10000;

  if(!op)
    {
      emitcode("ERROR", "    %s: op is null", __func__);
      return 0;
    }

  if(m6502_reg_x->isFree && m6502_reg_x->isDead)
    savereg=m6502_reg_x;
  else
    savereg=m6502_reg_y;


  if (!rematOfs && offset >= 0 && offset <= 0xff)
    {
      // no remat and 8-bit offset
      reg_info *reg0=m6502_findRegAop(AOP(op), 0);
      reg_info *reg1=m6502_findRegAop(AOP(op), 1);

      if ( IS_SAME_DPTR_OP(op) )
        {
          // do nothing
	  m6502_emitComment (TRACEGEN|VVDBG, "  %s: DPTR already has correct value", __func__);
        }
      else if(AOP_TYPE(op) == AOP_REG)
	{
	  m6502_emitComment (TRACEGEN|VVDBG, "    %s: AOP_REG", __func__);
	  storeRegToDPTR(AOP(op)->aopu.aop_reg[0], 0);
	  storeRegToDPTR(AOP(op)->aopu.aop_reg[1], 1);
	}
      else
        {
          reg_info *reg = NULL;

          m6502_emitComment (TRACEGEN|VVDBG, "    %s: not AOP_REG", __func__);

          if(reg0)
            storeRegToDPTR(reg0, 0);
          if(reg1)
            storeRegToDPTR(reg1, 1);
          if(reg0&&reg1)
            return offset;

	  if(m6502_reg_x->isFree && !keepTSX() )
	    reg=m6502_reg_x;
	  else if(m6502_reg_a->isFree && !savea)
	    reg=m6502_reg_a;
	  else if(m6502_reg_y->isFree)
	    reg=m6502_reg_y;

          // FIXME: save/restore x if SOF

          if(AOP(op)->type == AOP_SOF || reg==NULL)
            reg=m6502_reg_a;

          if(savea && reg==m6502_reg_a)
	    m6502_transferRegReg(m6502_reg_a, savereg, true);

	  if(!reg0)
	    {
	      m6502_loadRegFromAop(reg, AOP(op), 0);
	      storeRegToDPTR(reg, 0);
	    }

	  if(!reg1)
	    {
	      m6502_loadRegFromAop(reg, AOP(op), 1);
	      storeRegToDPTR(reg, 1);
            }

          if(savea && reg==m6502_reg_a)
            m6502_transferRegReg(savereg, m6502_reg_a, true);
          else
	    m6502_freeReg(m6502_reg_a);
	}
      return offset;
    }
  else
    {
      // general case
      m6502_emitComment (TRACEGEN|VVDBG, "    %s: general case", __func__);

      if(!rematOfs)
        rematOfs="0";

      if(savea)
        m6502_transferRegReg(m6502_reg_a, savereg, true);

      m6502_emitSetCarry(0);
      m6502_loadRegFromAop(m6502_reg_a, AOP(op), 0);
      m6502_emitOp ("adc", "#<(%s+%d)", rematOfs, offset);
      storeRegToDPTR(m6502_reg_a, 0);
      m6502_loadRegFromAop(m6502_reg_a, AOP(op), 1);
      m6502_emitOp ("adc", "#>(%s+%d)", rematOfs, offset);
      storeRegToDPTR(m6502_reg_a, 1);
      if(savea)
        m6502_transferRegReg(savereg, m6502_reg_a, true);
      else
        m6502_freeReg(m6502_reg_a);
      if(IS_AOP_XA(AOP(op)))
        m6502_freeReg(m6502_reg_x);
      return 0;
    }
}

/**************************************************************************
 * newAsmop - creates a new asmOp
 *************************************************************************/
static asmop *
newAsmop (short type)
{
  asmop *aop;
  // TODO: are these ever freed?
  aop = Safe_calloc (1, sizeof (asmop));
  aop->type = type;
  aop->op = NULL;
  return aop;
}

#if 0
/**************************************************************************
 * operandConflictsWithXY - true if operand in y and/or x register
 *************************************************************************/
static bool
operandConflictsWithXY (operand *op)
{
  symbol *sym;
  int i;

  if (IS_ITEMP (op))
    {
      sym = OP_SYMBOL (op);
      if (!sym->isspilt)
        {
          for(i = 0; i < sym->nRegs; i++)
            if (sym->regs[i] == m6502_reg_y || sym->regs[i] == m6502_reg_x)
              return true;
        }
    }

  return false;
}

/**************************************************************************
 * operandConflictsWithX - true if operand in x register
 *************************************************************************/
static bool
operandConflictsWithX (operand *op)
{
  symbol *sym;
  int i;

  if (IS_ITEMP (op))
    {
      sym = OP_SYMBOL (op);
      if (!sym->isspilt)
        {
          for(i = 0; i < sym->nRegs; i++)
            if (sym->regs[i] == m6502_reg_x)
              return true;
        }
    }

  return false;
}

/**************************************************************************
 * operandOnStack - returns True if operand is on the stack
 *************************************************************************/
static bool
operandOnStack(operand *op)
{
  symbol *sym;

  if (!op || !IS_SYMOP (op))
    return false;
  sym = OP_SYMBOL (op);
  if (!sym->isspilt && sym->onStack)
    return true;
  if (sym->isspilt)
    {
      sym = sym->usl.spillLoc;
      if (sym && sym->onStack)
        return true;
    }
  return false;
}

/**************************************************************************
 * tsxUseful - returns True if tsx could help at least one
 *             anticipated stack references
 *************************************************************************/
static bool
tsxUseful(const iCode *ic)
{
  operand *right  = IC_RIGHT(ic);
  operand *left   = IC_LEFT(ic);
  operand *result = IC_RESULT(ic);
  int uses = 0;

  if (ic->op == CALL)
    {
      if (result && operandSize (result) < 2 && operandOnStack (result))
        {
          uses++;
          ic = ic->next;
        }
    }

  while (ic && uses < 1)
    {
      if (ic->op == IFX)
	{
	  if (operandOnStack (IC_COND (ic)))
	    uses += operandSize(IC_COND (ic));
	  break;
	}
      else if (ic->op == JUMPTABLE)
	{
	  if (operandOnStack (IC_JTCOND (ic)))
	    uses++;
	  break;
	}
      else if (ic->op == ADDRESS_OF)
	{
	  if (operandOnStack (right))
	    break;
	}
      else if (ic->op == LABEL || ic->op == GOTO || ic->op == CALL || ic->op == PCALL)
	break;
      else if (POINTER_SET (ic) || POINTER_GET (ic))
	break;
      else
	{
	  if (operandConflictsWithXY (result))
	    break;
	  if (operandOnStack (left))
	    uses += operandSize (left);
	  if (operandOnStack (right))
	    uses += operandSize (right);
	  if (operandOnStack (result))
	    uses += operandSize (result);
	}

      ic = ic->next;
    }

  return uses >= 1;
}
#endif

bool
keepTSX()
{
  if(m6502_reg_x->aop==&m6502_tsxaop)
    return options.stackAuto || (currFunc && IFFUNC_ISREENT (currFunc->type));
  return false;
}

void
m6502_emitTSX()
{
  m6502_emitComment (TRACE_STACK|VVDBG, "%s: stackBase=%d Xofs=%d stackpush=%d",
		     __func__, _S.stackBase, m6502_reg_x->stackOffset, _S.stackPushes);

  // already did TSX
  if (m6502_reg_x->aop == &m6502_tsxaop)
    return;

  // put stack pointer in X
  if(!m6502_reg_x->isFree)
    emitcode("ERROR","m6502_emitTSX called with X in use");

  m6502_emitOp ("tsx", "");
}

// TODO: make these subroutines
static void saveBasePtr()
{
#if 0
  storeRegTemp (m6502_reg_x, true); // TODO: only when used?
  // TODO: if X is free should we call m6502_emitTSX() to mark X=S?
  m6502_emitTSX();
  m6502_emitOp ("stx", BASEPTR);
  _S.baseStackPushes = _S.stackPushes;
  m6502_loadRegTemp (m6502_reg_x);
#endif
}

static void
restoreBasePtr()
{
  // we recompute with saveBasePtr() after each jsr
}

/**************************************************************************
 * aopForSym - for a true symbol
 *************************************************************************/
static asmop * aopForSym (const iCode * ic, symbol * sym)
{
  asmop *aop;
  memmap *space;

  wassertl (ic != NULL, "Got a null iCode");
  wassertl (sym != NULL, "Got a null symbol");

  m6502_emitComment (TRACE_AOP|VVDBG, "%s", __func__);
      
  space = SPEC_OCLS (sym->etype);

  /* if already has one */
  if (sym->aop)
    {
      // sometimes the aop is stale. see comment in m6502_aopOp
      //  return sym->aop;
    }

  /* special case for a function */
  if (IS_FUNC (sym->type))
    {
      sym->aop = aop = newAsmop (AOP_IMMD);
      aop->aopu.aop_immd = Safe_calloc (1, strlen (sym->rname) + 1 + 6);
      sprintf (aop->aopu.aop_immd, "(%s)", sym->rname); // function pointer; take back one for RTS
      aop->size = FARPTRSIZE;
      return aop;
    }

  /* if it is on the stack */
  if (sym->onStack)
    {
      sym->aop = aop = newAsmop (AOP_SOF);
      aop->size = getSize (sym->type);
      aop->aopu.aop_stk = sym->stack;

      m6502_emitComment (TRACE_STACK|VVDBG, "%s: symbol %s: stack=%d size=%d", 
			 __func__, sym->name, sym->stack, aop->size);

#if 0
      if (!regalloc_dry_run && m6502_reg_x->isFree && m6502_reg_x->aop != &m6502_tsxaop) {
	if (!m6502_reg_x->isDead)
	  return aop;
	if (ic->op == IFX && operandConflictsWithX (IC_COND (ic)))
	  return aop;
	else if (ic->op == JUMPTABLE && operandConflictsWithX (IC_JTCOND (ic)))
	  return aop;
	else
          {
	    // FIXME: this is likely incorrect as XY is not a adr register in the 6502
	    /* If this is a pointer gen/set, then hx is definitely in use */
	    if (POINTER_SET (ic) || POINTER_GET (ic))
	      return aop;
	    if (ic->op == ADDRESS_OF)
	      return aop;
	    if (operandConflictsWithX (IC_LEFT (ic)))
	      return aop;
	    if (operandConflictsWithX (IC_RIGHT (ic)))
	      return aop;
	  }
	// TODO?
	/* It's safe to use tsx here. */
	if (!tsxUseful (ic))
	  return aop;
	// transfer S to X
	m6502_emitTSX();
      }
#endif
      return aop;
    }

  /* if it is in direct space */
  if (IN_DIRSPACE (space)) {
    sym->aop = aop = newAsmop (AOP_DIR);
    aop->aopu.aop_dir = sym->rname;
    aop->size = getSize (sym->type);
    return aop;
  }

  /* default to far space */
  sym->aop = aop = newAsmop (AOP_EXT);
  aop->aopu.aop_dir = sym->rname;
  aop->size = getSize (sym->type);
  return aop;
}

/**************************************************************************
 * aopForRemat - rematerializes an object
 *************************************************************************/
static asmop * aopForRemat (symbol * sym)
{
  iCode *ic = sym->rematiCode;
  asmop *aop = NULL;
  int val = 0;

  if (!ic) {
    fprintf (stderr, "Symbol %s to be rematerialized, but has no rematiCode.\n", sym->name);
    wassert (0);
  }

  for (;;) {
    if (ic->op == '+')
      val += (int) operandLitValue (IC_RIGHT (ic));
    else if (ic->op == '-')
      val -= (int) operandLitValue (IC_RIGHT (ic));
    else if (IS_CAST_ICODE (ic)) {
      ic = OP_SYMBOL (IC_RIGHT (ic))->rematiCode;
      continue;
    } else
      break;

    ic = OP_SYMBOL (IC_LEFT (ic))->rematiCode;
  }

  if (ic->op == ADDRESS_OF) {
    if (val) {
      SNPRINTF (buffer, sizeof (buffer),
                "(%s%c0x%04x)", OP_SYMBOL (IC_LEFT (ic))->rname, val >= 0 ? '+' : '-', abs (val) & 0xffff);
    } else {
      strncpyz (buffer, OP_SYMBOL (IC_LEFT (ic))->rname, sizeof (buffer));
    }

    aop = newAsmop (AOP_IMMD);
    aop->aopu.aop_immd = Safe_strdup (buffer);
  } else if (ic->op == '=') {
    val += (int) operandLitValue (IC_RIGHT (ic));
    val &= 0xffff;
    SNPRINTF (buffer, sizeof (buffer), "0x%04x", val);
    aop = newAsmop (AOP_LIT);
    aop->aopu.aop_lit = constVal (buffer);
  } else {
    werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "unexpected rematerialization");
  }

  return aop;
}

#if 0 // No longer used?
/**************************************************************************
 * regsInCommon - two operands have some registers in common
 *************************************************************************/
static bool
regsInCommon (operand * op1, operand * op2)
{
  symbol *sym1, *sym2;
  int i;

  /* if they have registers in common */
  if (!IS_SYMOP (op1) || !IS_SYMOP (op2))
    return false;

  sym1 = OP_SYMBOL (op1);
  sym2 = OP_SYMBOL (op2);

  if (sym1->nRegs == 0 || sym2->nRegs == 0)
    return false;

  for (i = 0; i < sym1->nRegs; i++)
    {
      int j;
      if (!sym1->regs[i])
        continue;

      for (j = 0; j < sym2->nRegs; j++)
        {
          if (!sym2->regs[j])
            continue;

          if (sym2->regs[j] == sym1->regs[i])
            return true;
        }
    }

  return false;
}
#endif

/**************************************************************************
 * operandsEqu - equivalent
 *************************************************************************/
static bool
operandsEqu (operand *op1, operand *op2)
{
  symbol *sym1, *sym2;

  /* if they not symbols */
  if (!IS_SYMOP (op1) || !IS_SYMOP (op2))
    return false;

  sym1 = OP_SYMBOL (op1);
  sym2 = OP_SYMBOL (op2);

  m6502_emitComment (TRACEGEN|VVDBG, "%s: sym1:%s(%d) sym2:%s(%d)", 
		     __func__, sym1->name, IS_ITEMP(op1),
		     sym2->name, IS_ITEMP(op2));

  /* if both are itemps & one is spilt
     and the other is not then false */
  if (IS_ITEMP (op1) && IS_ITEMP (op2) && sym1->isspilt != sym2->isspilt)
    return false;

  /* if they are the same */
  if (sym1 == sym2)
    return true;

  /* if they have the same rname */
  if (sym1->rname[0] && sym2->rname[0] && strcmp (sym1->rname, sym2->rname) == 0)
    return true;

  /* if left is a tmp & right is not */
  if (IS_ITEMP (op1) && !IS_ITEMP (op2) && sym1->isspilt && (sym1->usl.spillLoc == sym2))
    return true;

  if (IS_ITEMP (op2) && !IS_ITEMP (op1) && sym2->isspilt && sym1->level > 0 && (sym2->usl.spillLoc == sym1))
    return true;

  return false;
}

/**************************************************************************
 * m6502_sameRegs - two asmops have the same registers
 *************************************************************************/
bool
m6502_sameRegs (asmop * aop1, asmop * aop2)
{
  int i;

  if (aop1 == aop2)
    return true;

  //  if (aop1->size != aop2->size)
  //    return false;

  if (aop1->type == aop2->type) 
    {
      switch (aop1->type)
	{
	case AOP_REG:
	  for (i = 0; i < aop1->size; i++)
	    if (aop1->aopu.aop_reg[i] != aop2->aopu.aop_reg[i])
	      return false;
	  return true;
	case AOP_SOF:
	  return (aop1->aopu.aop_stk == aop2->aopu.aop_stk);
	case AOP_DIR:
	  //          if (regalloc_dry_run)
	  //            return false; // TODO: why?
	case AOP_EXT:
	  return (!strcmp (aop1->aopu.aop_dir, aop2->aopu.aop_dir));
	default:
	  break;
	}
    }

  return false;
}

/**************************************************************************
 * m6502_aopCanIncDec - asmop is EXT or DIR or X/Y
 *
 *************************************************************************/
bool
m6502_aopCanIncDec (asmop * aop)
{
  switch (aop->type)
    {
    case AOP_REG:
      if(aop->aopu.aop_reg[0]->rIdx == A_IDX)
        return IS_MOS65C02;
    case AOP_DIR:
    case AOP_EXT:
    case AOP_SOF:
      return true;
    default:
      break;
    }
  return false;
}

/**************************************************************************
 * m6502_aopCanShift - asmop is EXT or DIR or A
 *
 *************************************************************************/
bool
m6502_aopCanShift (asmop * aop)
{
  switch (aop->type) {
  case AOP_REG:
    return ((aop->size == 1) && (aop->aopu.aop_reg[0]->rIdx == A_IDX));
  case AOP_DIR:
  case AOP_EXT:
  case AOP_SOF:
    return true;
  default:
    break;
  }
  return false;
}

/**************************************************************************
 * m6502_aopCanBitOp - asmop is EXT or DIR
 *
 *************************************************************************/
bool
m6502_aopCanBit (asmop * aop)
{
  switch (aop->type)
    {
      // bit aa, bit aaaa
    case AOP_DIR:
    case AOP_EXT:
      return true;

      // bit #aa
    case AOP_LIT:
      return IS_MOS65C02;

      // TODO: ind,x for 65c02?
    default:
      break;
    }
  return false;
}

/**************************************************************************
 * addSign - complete with sign
 *************************************************************************/
//void
//addSign (operand * result, int offset, int sign)
//{
//
//}

/**************************************************************************
 * m6502_aopOp - allocates an asmop for an operand
 *************************************************************************/
void
m6502_aopOp (operand *op, const iCode * ic)
{
  asmop *aop = NULL;
  symbol *sym;
  int i;

  m6502_emitComment (TRACE_AOP, __func__);

  if (!op)
    return;

  /* if already has an asmop */
  if (op->aop)
    {
      op->aop=NULL;
#if 0
      m6502_emitComment (ALWAYS, "    %s: skip", __func__);
      if (IS_SYMOP (op) && OP_SYMBOL (op)->aop)
        {
          if(op->aop->type==AOP_SOF) {
            m6502_emitComment (VVDBG|TRACE_AOP, "    asmop symbol: %s [%d:%d] - %d",
			       OP_SYMBOL (op)->name, OP_SYMBOL (op)->stack, op->aop->size,
			       op->aop->aopu.aop_stk );
            // FIXME FIXME: occasionally the asmop has a stale value for the symbol
            // I am unable to trace the root cause, but forcing the symbol
            // to be reevaluated seems to fix all the regression failures
          }
        }
      return;
#endif
    }

  // Is this a pointer set result?
  if ((op == IC_RESULT (ic)) && POINTER_SET (ic))
    {
      m6502_emitComment (VVDBG|TRACE_AOP, "    %s: POINTER_SET", __func__);
    }

  /* if this a literal */
  if (IS_OP_LITERAL (op))
    {
      m6502_emitComment (VVDBG|TRACE_AOP, "    %s: LITERAL = 0x%x:%d",
			 __func__, ulFromVal(OP_VALUE (op)), getSize(operandType(op)) );
      aop = newAsmop (AOP_LIT);
      aop->aopu.aop_lit = OP_VALUE (op);
      aop->size = getSize (operandType (op));
      op->aop = aop;
      aop->op = op; // asmopToBool needs the op to check the type of the literal.
      return;
    }


  //  printf("checking underlying sym\n");
  /* if the underlying symbol has a aop */
#if 0
  // see the above comment on stale asmop
  if (IS_SYMOP (op) && OP_SYMBOL (op)->aop)
    {
      m6502_emitComment (VVDBG|TRACE_AOP, "    %s: SYMOP", __func__);
      op->aop = aop = Safe_calloc (1, sizeof (*aop));
      memcpy (aop, OP_SYMBOL (op)->aop, sizeof (*aop));
      //op->aop = aop = OP_SYMBOL (op)->aop;
      aop->size = getSize (operandType (op));
      m6502_emitComment (VVDBG|TRACE_AOP, "    symbol: %s [%d]",
			 OP_SYMBOL (op)->name, OP_SYMBOL (op)->stack );
      //printf ("reusing underlying symbol %s\n",OP_SYMBOL (op)->name);
      //printf (" with size = %d\n", aop->size);

      aop->op = op;
      return;
    }
#endif

  //  printf("checking true sym\n");
  /* if this is a true symbol */
  if (IS_TRUE_SYMOP (op))
    {
      m6502_emitComment (VVDBG|TRACE_AOP, "    %s: TRUE_SYMOP", __func__);
      op->aop = aop = aopForSym (ic, OP_SYMBOL (op));
      aop->op = op;
      //printf ("new symbol %s\n", OP_SYMBOL (op)->name);
      //printf (" with size = %d\n", aop->size);
      return;
    }

  /* this is a temporary : this has
     only five choices :
     a) register
     b) spillocation
     c) rematerialize
     d) conditional
     e) can be a return use only */

  m6502_emitComment (VVDBG|TRACE_AOP, "    %s: temp",
		     __func__);

  if (!IS_SYMOP (op))
    piCode (ic, NULL);
  sym = OP_SYMBOL (op);

  //  printf("checking conditional\n");
  /* if the type is a conditional */
  if (sym->regType == REG_CND)
    {
      m6502_emitComment (VVDBG|TRACE_AOP, "    %s: AOP_CRY",
			 __func__);
      sym->aop = op->aop = aop = newAsmop (AOP_CRY);
      aop->size = 0;
      aop->op = op;
      return;
    }

  //  printf("checking spilt\n");
  /* if it is spilt then two situations
     a) is rematerialize
     b) has a spill location */
  if (sym->isspilt || sym->nRegs == 0)
    {
      //      printf("checking remat\n");
      /* rematerialize it NOW */
      if (sym->remat)
        {
	  m6502_emitComment (VVDBG|TRACE_AOP, "    %s: remat",
			     __func__);
          sym->aop = op->aop = aop = aopForRemat (sym);
          aop->size = getSize (sym->type);
          aop->op = op;
          return;
        }

      wassertl (!sym->ruonly, "sym->ruonly not supported");

      if (regalloc_dry_run)
        {
          // Todo: Handle dummy iTemp correctly
          if (options.stackAuto || (currFunc && IFFUNC_ISREENT (currFunc->type)))
            {
              sym->aop = op->aop = aop = newAsmop (AOP_SOF);
              aop->aopu.aop_stk = 8; /* bogus stack offset, high enough to prevent optimization */
            }
          else
            {
              sym->aop = op->aop = aop = newAsmop (AOP_DIR);
              aop->aopu.aop_dir = sym->name; //TODO? avoids crashing in m6502_sameRegs()
            }
          aop->size = getSize (sym->type);
          aop->op = op;
          return;
        }

      /* else spill location  */
      if (sym->isspilt && sym->usl.spillLoc || regalloc_dry_run)
        {
          asmop *oldAsmOp = NULL;

	  m6502_emitComment (VVDBG|TRACE_AOP, "    %s: spill",
			     __func__);

          if (sym->usl.spillLoc->aop && sym->usl.spillLoc->aop->size != getSize (sym->type))
            {
              /* force a new aop if sizes differ */
              oldAsmOp = sym->usl.spillLoc->aop;
              sym->usl.spillLoc->aop = NULL;
              //printf ("forcing new aop\n");
            }
          sym->aop = op->aop = aop = aopForSym (ic, sym->usl.spillLoc);
          if (sym->usl.spillLoc->aop->size != getSize (sym->type))
            {
              /* Don't reuse the new aop, go with the last one */
              sym->usl.spillLoc->aop = oldAsmOp;
            }
          aop->size = getSize (sym->type);
          aop->op = op;
          //printf ("spill symbol %s\n", OP_SYMBOL (op)->name);
          //printf (" with size = %d\n", aop->size);
          return;
        }

      /* else must be a dummy iTemp */
      sym->aop = op->aop = aop = newAsmop (AOP_DUMMY);
      aop->size = getSize (sym->type);
      aop->op = op;
      return;
    }

  //  printf("assuming register\n");
  /* must be in a register */
  m6502_emitComment (VVDBG|TRACE_AOP, "    %s: nregs %d",
		     __func__, sym->nRegs );
  wassert (sym->nRegs);
  sym->aop = op->aop = aop = newAsmop (AOP_REG);
  aop->size = sym->nRegs;
  for (i = 0; i < sym->nRegs; i++)
    {
      wassert (sym->regs[i] >= m6502_regs && sym->regs[i] < m6502_regs + 3);
      wassertl (sym->regs[i], "Symbol in register, but no register assigned.");
      aop->aopu.aop_reg[i] = sym->regs[i];
      aop->regmask |= sym->regs[i]->mask;
    }
  //  if ((sym->nRegs > 1) && (sym->regs[0]->mask > sym->regs[1]->mask))
  //    aop->regmask |= M6502MASK_REV;
  aop->op = op;
}

/**************************************************************************
 * m6502_freeAsmop - free up the asmop given to an operand
 *************************************************************************/
void
m6502_freeAsmop (operand * op, asmop * aaop)
{
  asmop *aop;
  m6502_emitComment (TRACE_AOP, "%s", __func__);

  if (!op)
    aop = aaop;
  else
    aop = op->aop;

  if (!aop)
    return;

  if (aop->freed)
    goto dealloc;

  aop->freed = 1;

  if (aop->stacked)
    {
      int stackAdjust;
      int loffset;

      m6502_emitComment (TRACE_AOP, "%s -  m6502_freeAsmop restoring stacked %s", __func__, aopName (aop));
      aop->stacked = 0;
      stackAdjust = 0;
      for (loffset = 0; loffset < aop->size; loffset++)
	if (aop->stk_aop[loffset])
	  {
	    m6502_transferAopAop (aop->stk_aop[loffset], 0, aop, loffset);
	    stackAdjust++;
	  }
      pullNull (stackAdjust);
    }

 dealloc:
  /* all other cases just dealloc */
  if (op)
    {
      op->aop = NULL;
      if (IS_SYMOP (op))
	{
	  OP_SYMBOL (op)->aop = NULL;
	  /* if the symbol has a spill */
	  if (SPIL_LOC (op))
	    SPIL_LOC (op)->aop = NULL;
	}
    }
}


/**************************************************************************
 * aopDerefAop - treating the aop parameter as a pointer, return an asmop
 *               for the object it references
 *************************************************************************/
static asmop * aopDerefAop (asmop * aop, int offset)
{
  int adr;
  asmop *newaop = NULL;
  sym_link *type, *etype;
  int p_type;
  struct dbuf_s dbuf;

  m6502_emitComment (TRACE_AOP, "      aopDerefAop(%s)", aopName (aop));
  if (aop->op) {
    type = operandType (aop->op);
    etype = getSpec (type);
    /* if op is of type of pointer then it is simple */
    if (IS_PTR (type) && !IS_FUNC (type->next))
      p_type = DCL_TYPE (type);
    else
      {
        /* we have to go by the storage class */
        p_type = PTR_TYPE (SPEC_OCLS (etype));
      }
  }
  else
    p_type = UPOINTER;

  switch (aop->type) {
  case AOP_IMMD:
    if (p_type == POINTER)
      newaop = newAsmop (AOP_DIR);
    else
      newaop = newAsmop (AOP_EXT);
    if (!offset)
      newaop->aopu.aop_dir = aop->aopu.aop_immd;
    else {
      dbuf_init (&dbuf, 64);
      dbuf_printf (&dbuf, "(%s+%d)", aop->aopu.aop_immd, offset);
      newaop->aopu.aop_dir = dbuf_detach_c_str (&dbuf);
    }
    break;
  case AOP_LIT:
    adr = (int) ulFromVal (aop->aopu.aop_lit);
    if (p_type == POINTER)
      adr &= 0xff;
    adr = (adr + offset) & 0xffff;
    dbuf_init (&dbuf, 64);

    if (adr < 0x100) {
      newaop = newAsmop (AOP_DIR);
      dbuf_printf (&dbuf, "0x%02x", adr);
    } else {
      newaop = newAsmop (AOP_EXT);
      dbuf_printf (&dbuf, "0x%04x", adr);
    }
    newaop->aopu.aop_dir = dbuf_detach_c_str (&dbuf);
    break;
  default:
    werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "unsupported asmop");
    return NULL;
  }

  return newaop;
}

// is it safe to aopAdrStr?
static bool
isAddrSafe(operand* op, reg_info* reg)
{
  switch (AOP(op)->type)
    {
    case AOP_IMMD:    // #nn
    case AOP_LIT:
    case AOP_DIR:     // aa
    case AOP_EXT:     // aaaa
      return true;
    case AOP_SOF:     // SOF,x
      if (reg == m6502_reg_a && (m6502_reg_x->isFree || m6502_reg_y->isFree))
	return true;
    default:
      break;
    }
  return false;
}

static int aopPrepareStoreTemp = 0;
static int aopPreparePreserveFlags = 0;

// TODO: make sure this is called before/after aopAdrStr if indexing might be used
static void
aopAdrPrepare (asmop * aop, int loffset)
{
  m6502_emitComment (TRACE_AOP, "%s", __func__ );

  aopPreparePreserveFlags = 0;
  if (loffset > (aop->size - 1))
    return;

  if (aop->type==AOP_SOF) {
#if 0
    // code for lda [BASEPTR],y
    aopPrepareStoreTemp = storeRegTemp(m6502_reg_y, false);
    // FIXME: offset is wrong
    m6502_emitComment (TRACE_AOP, "ofs=%d base=%d tsx=%d push=%d stk=%d loffset=%d", _S.stackBase, _S.baseStackPushes, reg->stackOffset, _S.stackPushes, aop->aopu.aop_stk, loffset);
    m6502_loadRegFromConst(m6502_reg_y, _S.stackBase + _S.baseStackPushes + aop->aopu.aop_stk + loffset + 1);
    // ORIG: m6502_loadRegFromConst(m6502_reg_y, _S.stackBase - _S.baseStackPushes + aop->aopu.aop_stk + loffset + 1);
    m6502_reg_y->aop = &m6502_tsxaop;
#else
    // can we get stack pointer?
    aopPrepareStoreTemp=0;
    if (!m6502_reg_x->isFree) {
      // FIXME: check if used/dead is ok
      // aopPrepareStoreTemp = storeRegTempIfSurv(m6502_reg_x);
      if (m6502_reg_x->aop != &m6502_tsxaop)
        {
	  m6502_emitComment (TRACE_AOP, "    aopAdrPrepare: x!=m6502_tsxaop");
          storeRegTemp(m6502_reg_x, true);
          aopPrepareStoreTemp = true;
          // m6502_useReg(m6502_reg_x);
        }
    }

    m6502_emitTSX();
#endif
    aopPreparePreserveFlags = 1; // TODO: also need to make sure flags are needed by caller
  }
}

static void
aopAdrUnprepare (asmop * aop, int loffset)
{
  if (loffset > (aop->size - 1))
    return;

  if (aop->type==AOP_SOF) {
    if (aopPrepareStoreTemp) {
      if (aopPreparePreserveFlags)
	m6502_loadRegTempNoFlags(m6502_reg_x, true);
      else
	m6502_loadRegTemp(m6502_reg_x);

      aopPreparePreserveFlags = 0;
      aopPrepareStoreTemp = 0;
    }
  }
}


/**************************************************************************
 * aopAdrStr - for referencing the address of the aop
 *************************************************************************/
/* loffset seems to have a weird meaning here. It seems to be nonzero in some places where one would expect an offset to be zero */
static const char *
aopAdrStr (asmop * aop, int loffset, bool bit16)
{
  char *s = buffer;
  char *rs;
  int offset = loffset; // SEH: aop->size - 1 - loffset - (bit16 ? 1 : 0);
  int xofs;

  m6502_emitComment(VVDBG|TRACEGEN,"      %s: size=%d offs=%d",
		    __func__, aop->size, offset);

  /* offset is greater than
     size then zero */
  if (loffset > (aop->size - 1) && aop->type != AOP_LIT)
    return "#0x00";

  /* depending on type */
  switch (aop->type)
    {
    case AOP_DUMMY:
      return "#0x00";

    case AOP_REG:
      return aop->aopu.aop_reg[loffset]->name;

    case AOP_IMMD:
      if (regalloc_dry_run)
	return "#0x00";

      if (loffset)
	{
	  if (loffset > 1)
	    sprintf (s, "#(%s >> %d)", aop->aopu.aop_immd, loffset * 8);
	  else
	    sprintf (s, "#>%s", aop->aopu.aop_immd);
	}
      else
	sprintf (s, "#%s", aop->aopu.aop_immd);

      rs = Safe_calloc (1, strlen (s) + 1);
      strcpy (rs, s);
      return rs;

    case AOP_LIT:
      if (bit16)
	return aopLiteralLong (aop->aopu.aop_lit, loffset, 2);
      else
	return aopLiteral (aop->aopu.aop_lit, loffset);

    case AOP_DIR:
      if (regalloc_dry_run)
	return "*dry";
      if (offset)
	sprintf (s, "*(%s+%d)", aop->aopu.aop_dir, offset);
      else
	sprintf (s, "*%s", aop->aopu.aop_dir);
      rs = Safe_calloc (1, strlen (s) + 1);
      strcpy (rs, s);
      return rs;

    case AOP_EXT:
      if (regalloc_dry_run)
	return "dry";
      if (offset)
	sprintf (s, "(%s+%d)", aop->aopu.aop_dir, offset);
      else
	sprintf (s, "%s", aop->aopu.aop_dir);
      rs = Safe_calloc (1, strlen (s) + 1);
      strcpy (rs, s);
      return rs;

    case AOP_SOF: // TODO?
      //   if (regalloc_dry_run)
      //	{
      //	  m6502_emitTSX();
      //	  return "0x100,x"; // fake result, not needed
      //	}
      //      else
      {
	// FIXME FIXME: force emit of TSX to avoid offset < 0x100
	// this is a workaround for the assembler incorrectly
	// generating ZP,x instead of ABS,x
	if((_S.stackBase - m6502_reg_x->stackOffset + aop->aopu.aop_stk + offset + 1)<0)
	  {
	    m6502_dirtyReg(m6502_reg_x);
	  }
	// FIXME: this is usually redundant as it is explicitly called
	// before calling aopAdrStr
	m6502_emitTSX();

	xofs = STACK_TOP + _S.stackBase - m6502_reg_x->stackOffset + aop->aopu.aop_stk + offset + 1;

	m6502_emitComment(ALWAYS,"      op target: SP+%d+%d [base:%d + off:%d + sym:%d - reg:%d + 1 ] -> %d (0x%02x)",
			  _S.stackBase + aop->aopu.aop_stk + 1, offset,
			  _S.stackBase, offset,
			  aop->aopu.aop_stk, m6502_reg_x->stackOffset, 
			  xofs - STACK_TOP, xofs - STACK_TOP);

	sprintf (s, IDXFMT_X, xofs);
	rs = Safe_calloc (1, strlen (s) + 1);
	strcpy (rs, s);
	return rs;
#if 0
 	else if (m6502_reg_y->aop == &m6502_tsxaop) {
	  return "[__BASEPTR],y";
	} else {
	  // FIXME: unimplemented
	  //          m6502_loadRegFromConst(m6502_reg_x, offset);
	  return "ERROR [__BASEPTR],y"; // TODO: is base ptr or Y loaded?
	}
#endif
      }
    default:
      break;
    }
  return "ERROR - aopAdrStr: unknown aop type";
}

/**************************************************************************
 * asmopToBool - Emit code to convert an asmop to a boolean.
 *               Result left in A (0=false, 1=true) if ResultInA,
 *               otherwise result left in Z flag (1=false, 0=true)
 *************************************************************************/
static void
asmopToBool (asmop *aop, bool resultInA)
{
  symbol *tlbl = m6502_safeNewiTempLabel (NULL);
  sym_link *type;
  int size = aop->size;
  int offset = size - 1;
  bool needloada = false;
  bool isFloat;

  m6502_emitComment (TRACEGEN, "  %s - size:%d resultinA: %s",
		     __func__, size, resultInA?"yes":"no");

  wassert (aop);
  type = operandType (AOP_OP (aop));
  isFloat = IS_FLOAT (type);

  if (resultInA)
    m6502_freeReg (m6502_reg_a);

  if (IS_BOOL (type))
    {
      if (resultInA)
        {
          // result -> A
          m6502_loadRegFromAop (m6502_reg_a, aop, 0);
        }
      else
        {
          // result -> flags
          if (aop->type==AOP_REG)
            {
	      m6502_emitCmp(aop->aopu.aop_reg[0], 0);
            }
          else
            {
	      reg_info* freereg = m6502_getDeadByteReg();
              if (freereg)
                {
                  m6502_loadRegFromAop (freereg, aop, 0);
		  m6502_emitCmp(freereg, 0x00);
                }
              else
                {
                  // no choice, all regs are full
                  storeRegTemp (m6502_reg_a, true);
                  m6502_loadRegFromAop (m6502_reg_a, aop, 0);
                  m6502_loadRegTempNoFlags (m6502_reg_a, true);
                }
            }
        }
      return;
    }

#if 0 
  if (resultInA && size == 1 && !IS_AOP_A(aop) /*_S.lastflag!=A_IDX*/)
    {
      m6502_loadRegFromAop (m6502_reg_a, aop, 0);
      m6502_emitCmp(m6502_reg_a, 0x01);
      m6502_loadRegFromConst (m6502_reg_a, 0);
      m6502_rmwWithReg ("rol", m6502_reg_a);
      return;
    }
#endif

  if(resultInA && size==1)
    {
      if(IS_AOP_A(aop))
	m6502_emitCmp(m6502_reg_a, 0);
      else
	m6502_loadRegFromAop (m6502_reg_a, aop, 0);
      goto end;
    }

  switch (aop->type)
    {
    case AOP_REG:
      if (size==1)
        {  // A, X or Y
	  m6502_emitCmp(aop->aopu.aop_reg[0], 0);
          return;
        }
      else if(size==2 && m6502_reg_x->isLitConst)
        {
          if(m6502_reg_x->litConst!=0)
            {
              if(resultInA)
                m6502_loadRegFromConst (m6502_reg_a, 1);
              else
                m6502_emitCmp(m6502_reg_x, 0);

              return;
            }
          else
            {
              if (IS_AOP_XA (aop))
                m6502_emitCmp(m6502_reg_a, 0);
              if (IS_AOP_XY (aop))
                m6502_emitCmp(m6502_reg_y, 0);
            }
        }
      else if (IS_AOP_XA (aop))
        {
#if 0
	  // FIXME: this optimization makes the code smaller and slower
	  // should consider for size optimization

	  if(m6502_reg_a->isDead && _S.lastflag!=A_IDX && _S.lastflag!=X_IDX)
	    {
	      storeRegTemp(m6502_reg_x, false);
	      m6502_emitRegTempOp("ora", m6502_getLastTempOfs() );
	      m6502_loadRegTemp (NULL);
	    }
	  else
#endif

	    if(_S.lastflag==X_IDX) 
	      {
		m6502_emitBranch ("bne", tlbl);
		m6502_emitCmp(m6502_reg_a, 0);
	      }
	    else
	      {
		m6502_emitCmp(m6502_reg_a, 0);
                m6502_emitBranch ("bne", tlbl);
		// FIXME: this optimization makes the code smaller (expected) and slower (unexpected)
		//          if(m6502_reg_a->isDead) 
		//            m6502_transferRegReg(m6502_reg_x, m6502_reg_a, true);
		//          else 
		m6502_emitCmp(m6502_reg_x, 0);
	      }
        }
      else if (IS_AOP_XY (aop))
        {
          if(resultInA)
            m6502_loadRegFromConst (m6502_reg_a, 0);

	  m6502_emitCmp(m6502_reg_x, 0);
          m6502_emitBranch ("bne", tlbl);
	  m6502_emitCmp(m6502_reg_y, 0);
        }
      else
        {
          emitcode("ERROR", "Bad %02x regmask in asmopToBool", (aop)->regmask);
          return;
        }
      break;

    case AOP_LIT:
      /* Higher levels should optimize this case away but let's be safe */
      if (ulFromVal (aop->aopu.aop_lit))
        m6502_loadRegFromConst (m6502_reg_a, 1);
      else
        m6502_loadRegFromConst (m6502_reg_a, 0);
      m6502_freeReg (m6502_reg_a);
      break;

    case AOP_DIR:
    case AOP_EXT:
      m6502_emitComment (TRACE_AOP|VVDBG, "  %s - AOP_DIR || AOP_EXT", __func__);

#if 1
      if (!resultInA && (size == 1) && !IS_AOP_A (aop) && !m6502_reg_a->isFree && m6502_reg_x->isFree)
        {
          m6502_emitComment (TRACE_AOP|VVDBG, "  %s - load", __func__);
          m6502_loadRegFromAop (m6502_reg_x, aop, 0);
          return;
        }
#else
      if (!resultInA && (size == 1) )
        {
          reg_info *reg=m6502_getFreeByteReg();
          m6502_emitComment (TRACE_AOP|VVDBG, "  %s - reg:%s", __func__,(reg)?reg->name:"NULL");

          if(reg)
            {
              m6502_loadRegFromAop (reg, aop, 0);
	      m6502_emitCmp(reg, 0x00);
              return;
            }
        }
#endif 

    default:
      if (!resultInA)
        needloada = storeRegTempIfSurv(m6502_reg_a);

      m6502_loadRegFromAop (m6502_reg_a, aop, size-1);
      if (isFloat)
        m6502_emitOp ("and", "#0x7F");
      else if(size==1)
	m6502_emitCmp(m6502_reg_a, 0x00);

      for(offset=size-2; offset>=0; offset--)
        m6502_accopWithAop ("ora", aop, offset);

      if (!resultInA)
        {
	  m6502_loadRegTempNoFlags (m6502_reg_a, needloada);
          return;
        }
      else
        {
          m6502_freeReg (m6502_reg_a);
        }
      break;
    }

 end:
  if (resultInA)
    {
      symbol *skiplbl = m6502_safeNewiTempLabel (NULL);

      m6502_emitBranch ("beq", skiplbl);
      m6502_safeEmitLabel (tlbl);
      m6502_loadRegFromConst (m6502_reg_a, 1);
      m6502_safeEmitLabel (skiplbl);
      _S.lastflag=A_IDX;
      m6502_dirtyReg (m6502_reg_a);
      m6502_useReg (m6502_reg_a);
    }
  else
    {
      if(size>1)
        m6502_safeEmitLabel (tlbl);
    }
}

/**************************************************************************
 * m6502_copy - Copy the value from one operand to another
 *           The caller is responsible for m6502_aopOp and m6502_freeAsmop
 *************************************************************************/
void
m6502_copy (operand * result, operand * source)
{
  int size = AOP_SIZE (result);
  int srcsize = AOP_SIZE (source);
  int offset = 0;

  m6502_emitComment (TRACEGEN, __func__);
  m6502_emitComment (TRACEGEN|VVDBG, "      %s - size %d -> %d", __func__, srcsize, size);
  m6502_emitComment (TRACEGEN|VVDBG, "      %s - regmask %02x -> %02x",
		     __func__, AOP(source)->regmask, AOP(result)->regmask );

  /* if they are the same and not volatile */
  if (operandsEqu (result, source) && !isOperandVolatile (result, false) &&
      !isOperandVolatile (source, false))
    return;

  /* if they are the same registers */
  if (m6502_sameRegs (AOP (source), AOP (result)) && srcsize == size )
    return;

  if(IS_AOP_XA (AOP (result)) )
    {
      m6502_loadRegFromAop (m6502_reg_xa, AOP(source), 0);
      return;
    }

  if(IS_AOP_XY (AOP (result)) )
    {
      m6502_loadRegFromAop (m6502_reg_xy, AOP(source), 0);
      return;
    }

  if (IS_AOP_XA (AOP (source)) && size <= 2  )
    {
      m6502_storeRegToAop (m6502_reg_xa, AOP (result), 0);
      return;
    }

  if (IS_AOP_XY (AOP (source)) && size <= 2  )
    {
      m6502_storeRegToAop (m6502_reg_xy, AOP (result), 0);
      return;
    }

  if(srcsize==1 && AOP_TYPE(result) != AOP_SOF)
    {
      reg_info *reg0=m6502_findRegAop(AOP(source), 0);
      if(reg0)
        {
          int i;
          m6502_emitComment (TRACEGEN|VVDBG, "      %s (srcsize = 1)", __func__);
	  m6502_storeRegToAop (reg0, AOP(result), 0);
          for(i=1;i<size;i++)
            m6502_storeConstToAop(0,AOP(result),i);

          return;
        }
    }

  if(size==2 && AOP_TYPE(result) != AOP_SOF)
    {
      reg_info *reg0=m6502_findRegAop(AOP(source), 0);
      reg_info *reg1=m6502_findRegAop(AOP(source), 1);
      if(reg0&&reg1)
        {
          m6502_emitComment (TRACEGEN|VVDBG, "      %s (regtrack)", __func__);
	  m6502_storeRegToAop (reg0, AOP(result), 0);
	  m6502_storeRegToAop (reg1, AOP(result), 1);
          return;
        }
    }

#if 1
  if(AOP_TYPE (source) == AOP_SOF || AOP_TYPE(result) == AOP_SOF)
    {
      m6502_emitComment (TRACEGEN|VVDBG, "      %s (SOF)", __func__);
      bool save_a, save_x;
      save_a = storeRegTempIfSurv(m6502_reg_a);
      save_x = storeRegTempIfSurv(m6502_reg_x);

      for(offset=size-1; offset>=0; offset--)
	{
	  if(offset >= srcsize)
	    {
	      m6502_loadRegFromConst (m6502_reg_a, 0);
	      m6502_storeRegToAop (m6502_reg_a, AOP(result), offset);
	      m6502_freeReg(m6502_reg_a);
	    }
	  else
	    {
	      m6502_loadRegFromAop (m6502_reg_a, AOP(source), offset);
	      m6502_storeRegToAop (m6502_reg_a, AOP(result), offset);
	      m6502_freeReg(m6502_reg_a);
	    }
	}
      m6502_loadOrFreeRegTemp(m6502_reg_x, save_x);
      m6502_loadOrFreeRegTemp(m6502_reg_a, save_a);

      return;
    }
#endif

  /* general case */
  m6502_emitComment (TRACEGEN|VVDBG, "      %s (general case)", __func__);

  if(m6502_findRegAop (AOP(source), 0))
    {
      for(offset=0; offset<srcsize; offset++)
	m6502_transferAopAop (AOP (source), offset, AOP (result), offset);
      for( ; offset<size; offset++)
	m6502_storeConstToAop (0, AOP (result), offset);
    }
  else
    {
      for(offset=size-1; offset>=srcsize; offset--)
	m6502_storeConstToAop (0, AOP (result), offset);
      for( ; offset>=0; offset--)
	m6502_transferAopAop (AOP (source), offset, AOP (result), offset);
    }

}

/**************************************************************************
 * genNot - generate code for ! operation
 *************************************************************************/
static void
genNot (iCode * ic)
{
  operand *left = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);
  bool needpulla;

  m6502_emitComment (TRACEGEN, __func__);
  m6502_printIC(ic);

  /* assign asmOps to operand & result */
  m6502_aopOp (left, ic);
  m6502_aopOp (result, ic);

  needpulla = pushRegIfSurv (m6502_reg_a);
  asmopToBool (AOP (left), true);
  m6502_emitOp ("eor", "#0x01");
  m6502_storeRegToFullAop (m6502_reg_a, AOP (result), false);
  pullOrFreeReg (m6502_reg_a, needpulla);

  m6502_freeAsmop (result, NULL);
  m6502_freeAsmop (left, NULL);
}

/**************************************************************************
 * genUminusFloat - unary minus for floating points
 *************************************************************************/
static void
genUminusFloat (operand * op, operand * result)
{
  int size, offset;
  bool needpulla;

  m6502_emitComment (TRACEGEN, __func__);

  /* for this we just copy and then flip the bit */
  size = AOP_SIZE (op);
  needpulla = pushRegIfSurv (m6502_reg_a);

  for(offset=0; offset<size-1; offset++)
    m6502_transferAopAop (AOP (op), offset, AOP (result), offset);

  m6502_loadRegFromAop (m6502_reg_a, AOP (op), size-1);
  m6502_emitOp ("eor", "#0x80");
  m6502_storeRegToAop (m6502_reg_a, AOP (result), size-1);
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * genUminus - unary minus code generation
 *************************************************************************/
static void genUminus (iCode * ic)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int offset, size;
  sym_link *optype;
  bool carry = true;
  bool needpula = false;

  m6502_emitComment (TRACEGEN, __func__);
  m6502_printIC (ic);

  /* assign asmops */
  m6502_aopOp (left, ic);
  m6502_aopOp (result, ic);

  optype = operandType (left);

  sym_link *resulttype = operandType (IC_RESULT (ic));
  unsigned topbytemask = (IS_BITINT (resulttype) && SPEC_USIGN (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;
  bool maskedtopbyte = (topbytemask != 0xff);

  /* if float then do float stuff */
  if (IS_FLOAT (optype))
    {
      genUminusFloat (left, result);
      goto release;
    }

  /* otherwise subtract from zero */
  size = AOP_SIZE (left);

  if (size == 1)
    {
      needpula = pushRegIfSurv (m6502_reg_a);
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), 0);
      m6502_rmwWithReg ("neg", m6502_reg_a);
      if (maskedtopbyte)
        m6502_emitOp ("and", IMMDFMT, topbytemask);
      //      m6502_freeReg (m6502_reg_a);
      m6502_storeRegToFullAop (m6502_reg_a, AOP (result), SPEC_USIGN (operandType (left)));
      goto release;
    }

  /* If either left or result are in registers, handle this carefully to     */
  /* avoid prematurely overwriting register values. The 1 byte case was      */
  /* handled above and there aren't enough registers to handle 4 byte values */
  /* so this case only needs to deal with 2 byte values. */
  if (AOP_TYPE (result) == AOP_REG || AOP_TYPE (left) == AOP_REG)
    {
      reg_info *result0 = NULL;
      reg_info *left0 = NULL;
      reg_info *left1 = NULL;
      if (AOP_TYPE (left) == AOP_REG)
        {
          left0 = AOP (left)->aopu.aop_reg[0];
          left1 = AOP (left)->aopu.aop_reg[1];
        }
      if (AOP_TYPE (result) == AOP_REG)
        {
          result0 = AOP (result)->aopu.aop_reg[0];
        }
      needpula = pushRegIfSurv (m6502_reg_a);
      if (left1 == m6502_reg_a)
        m6502_pushReg (left1, true);

      if (left0 == m6502_reg_a) // TODO?
        m6502_rmwWithReg ("neg", m6502_reg_a);
      else {
        m6502_loadRegFromConst (m6502_reg_a, 0);
        m6502_emitSetCarry(1);
        m6502_accopWithAop ("sbc", AOP (left), 0);
      }
      if (result0 == m6502_reg_a || (result0 && result0 == left1))
        m6502_pushReg (m6502_reg_a, true);
      else
        m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);

      m6502_loadRegFromConst (m6502_reg_a, 0);
      if (left1 == m6502_reg_a)
	{
	  // FIXME: unimplemented
	  m6502_unimplemented ("genUniminus with left1=A");
	  m6502_dirtyReg (m6502_reg_a);
	}
      else
	{
	  m6502_accopWithAop ("sbc", AOP (left), 1);
	}
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
      if (result0 == m6502_reg_a || (result0 && result0 == left1))
        m6502_pullReg (result0);
      if (left1 == m6502_reg_a)
        pullNull (1);
      goto release;
    }

  needpula = pushRegIfSurv (m6502_reg_a);
  for (offset=0; offset<size; offset++)
    {
      m6502_loadRegFromConst (m6502_reg_a, 0);
      if (carry)
	{
	  m6502_emitSetCarry(1);
	}
      m6502_accopWithAop ("sbc", AOP (left), offset);
      m6502_storeRegToAop (m6502_reg_a, AOP(result), offset);
      carry = false;
    }
  //  storeRegSignToUpperAop (m6502_reg_a, AOP(result), offset, SPEC_USIGN (operandType (left)));

 release:
  pullOrFreeReg (m6502_reg_a, needpula);
  m6502_freeAsmop (result, NULL);
  m6502_freeAsmop (left, NULL);
}

/**************************************************************************
 * saveRegisters - will look for a call and save the registers
 *************************************************************************/
static void
saveRegisters (iCode *lic)
{
  int i;
  iCode *ic;

  /* look for call */
  for (ic = lic; ic; ic = ic->next)
    if (ic->op == CALL || ic->op == PCALL)
      break;

  if (!ic)
    {
      fprintf (stderr, "found parameter push with no function call\n");
      return;
    }

  /* if the registers have been saved already or don't need to be then
     do nothing */
  if (ic->regsSaved)
    return;

  if (IS_SYMOP (IC_LEFT (ic)) &&
      (IFFUNC_CALLEESAVES (OP_SYMBOL (IC_LEFT (ic))->type) || IFFUNC_ISNAKED (OP_SYM_TYPE (IC_LEFT (ic)))))
    return;

  if (!regalloc_dry_run)
    ic->regsSaved = 1;

  m6502_emitComment (REGOPS|VVDBG, "  %s - Not previously saved", __func__);

  // make sure not to clobber A
  // TODO: why does isUsed not set?
  // TODO: only clobbered if m6502_reg_a->isFree

  if(bitVectBitValue(ic->rSurv, A_IDX) || bitVectBitValue(ic->rSurv, X_IDX)
     || bitVectBitValue(ic->rSurv, Y_IDX))
    {
      m6502_emitComment (TRACEGEN, "  %s - must save %c%c%c", __func__,
			 bitVectBitValue(ic->rSurv, A_IDX)?'A':'-',
			 bitVectBitValue(ic->rSurv, X_IDX)?'X':'-',
			 bitVectBitValue(ic->rSurv, Y_IDX)?'Y':'-'  );
      //  m6502_emitOp("nop", "");
      // regalloc_dry_run_cost_cycles+=50;
    }


  bool clobbers_a = !IS_MOS65C02
    && (bitVectBitValue(ic->rSurv, X_IDX) || bitVectBitValue(ic->rSurv, Y_IDX))
    && !bitVectBitValue(ic->rSurv, A_IDX);

  if (clobbers_a && _S.sendSet)
    storeRegTemp (m6502_reg_a, true);

  for (i = A_IDX; i <= Y_IDX; i++)
    {
      if (bitVectBitValue (ic->rSurv, i))
        {
	  m6502_pushReg (m6502_regWithIdx (i), false);
	  //       if(i==Y_IDX)
	  //         m6502_reg_y->isDead=true;
        }
    }

  if (clobbers_a && _S.sendSet)
    m6502_loadRegTemp (m6502_reg_a);
}

/**************************************************************************
 * unsaveRegisters - pop the pushed registers
 *************************************************************************/
static void unsaveRegisters (iCode *ic)
{
  int i;

  m6502_emitComment (REGOPS, "%s", __func__);

  // TODO: only clobbered if m6502_reg_a->isFree

  bool clobbers_a = !IS_MOS65C02
    && (bitVectBitValue(ic->rSurv, X_IDX) || bitVectBitValue(ic->rSurv, Y_IDX))
    && !bitVectBitValue(ic->rSurv, A_IDX);

  if (clobbers_a)
    storeRegTemp (m6502_reg_a, true);

  for (i = Y_IDX; i >= A_IDX; i--)
    {
      if (bitVectBitValue (ic->rSurv, i))
	m6502_pullReg (m6502_regWithIdx (i));
    }
  if (clobbers_a)
    m6502_loadRegTemp (m6502_reg_a);
}

/**************************************************************************
 * storeOperToDPTR
 * store oper to DPTR
 *************************************************************************/
static void
storeOperToDPTR (operand *oper, int size, iCode *ic)
{
  reg_info *reg = NULL;
  int offset;
  bool needloada = false;
  bool needloadx = false;

  m6502_aopOp (oper, ic);

  if (AOP_TYPE (oper) == AOP_REG)
    {
      /* The operand is in registers; we can save them directly */
      storeRegToDPTR (AOP (oper)->aopu.aop_reg[0], 0);
      storeRegToDPTR (AOP (oper)->aopu.aop_reg[1], 1);
    }
  else
    {
      if(AOP_TYPE(oper)!=AOP_SOF)
        reg=m6502_getFreeByteReg();

      if(!reg)
        {
          // push A if not free
          reg=m6502_reg_a;
          needloada = pushRegIfUsed(m6502_reg_a);
        }

      if(AOP_TYPE(oper)==AOP_SOF)
        needloadx=pushRegIfUsed(m6502_reg_x);

      /* A is free, so piecewise load operand into a and push A */
      for (offset=0; offset<size; offset++)
	{
	  m6502_loadRegFromAop (reg, AOP (oper), offset);
          storeRegToDPTR (reg, offset);
	}

      pullOrFreeReg (m6502_reg_x, needloadx);
      pullOrFreeReg (m6502_reg_a, needloada);
    }

  m6502_freeAsmop (oper, NULL);
}

/**************************************************************************
 * assignResultValue
 *************************************************************************/
static void
assignResultValue (operand * oper)
{
  int size = AOP_SIZE (oper);
  int offset;

  m6502_emitComment (TRACEGEN, "%s - size:%d",
		     __func__, size);
  if (size>8)
    {
      emitcode("ERROR","assignresultvalue return struct size: %d\n",size);
      return;
    }

  if (AOP_TYPE (oper) == AOP_REG)
    {
      if(size==1)
        {
          m6502_transferRegReg (m6502_reg_a, (AOP(oper))->aopu.aop_reg[0], true);
        }
      else
        {
          if (IS_AOP_XY(AOP(oper)))
            m6502_transferRegReg(m6502_reg_xa, m6502_reg_xy, true);
        }
      return;
    }

  if (AOP_TYPE (oper)==AOP_SOF && size>1)
    {
      int i;
      storeRegTemp (m6502_reg_x, true);
      for(i=0; i<size; i++)
	{
	  if (i==1)
	    {
	      m6502_loadRegTemp(m6502_reg_a);
	      m6502_storeRegToAop (m6502_reg_a, AOP (oper), i);
	    }
	  else
	    m6502_transferAopAop (m6502_aop_pass[i], 0, AOP (oper), i);
	}  
    }
  else
    {
      for(offset=0; offset<size; offset++)
 	{
	  m6502_transferAopAop (m6502_aop_pass[offset], 0, AOP (oper), offset);
	  if (m6502_aop_pass[offset]->type == AOP_REG)
	    m6502_freeReg (m6502_aop_pass[offset]->aopu.aop_reg[0]);
	}
    }
}

/**************************************************************************
 * genIpush - generate code for pushing
 *************************************************************************/
static void
genIpush (iCode * ic)
{
  operand *left   = IC_LEFT (ic);
  int size, offset;

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (left, ic);

  /* if this is not a parm push : ie. it is spill push
     and spill push is always done on the local stack */
  if (!ic->parmPush)
    {
      /* and the item is spilt then do nothing */
      if (OP_SYMBOL (left)->isspilt)
	goto release;

      size = AOP_SIZE (left);
      /* push it on the stack */
      for (offset=size-1; offset>=0; offset--)
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
	  m6502_pushReg (m6502_reg_a, true);
	}
      goto release;
    }

  /* this is a parameter push: in this case we call
     the routine to find the call and save those
     registers that need to be saved */
  if (!regalloc_dry_run) /* Cost for saving registers is counted at CALL or PCALL */
    saveRegisters (ic);

  /* then do the push */
  size = AOP_SIZE (left);

  if (AOP_TYPE (left) == AOP_REG)
    {
      if (IS_AOP_XA (AOP (left)))
	{
	  m6502_pushReg (m6502_reg_xa, true);
	}
      else
	{
	  for (offset=size-1; offset>=0; offset--)
	    m6502_pushReg (AOP (left)->aopu.aop_reg[offset], true);
	}
      goto release;
    }

  bool needpulla = false;
  bool needpullx = false;
  if(AOP_TYPE(left)==AOP_SOF)
    needpullx=storeRegTempIfSurv(m6502_reg_x);

  needpulla=storeRegTempIfSurv(m6502_reg_a);

  for (offset=size-1; offset>=0; offset--)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
      m6502_pushReg (m6502_reg_a, true);
    }

  m6502_loadOrFreeRegTemp(m6502_reg_a, needpulla);
  m6502_loadOrFreeRegTemp(m6502_reg_x, needpullx);

 release:
  m6502_freeAsmop (left, NULL);
}

/**************************************************************************
 * genPointerPush - generate code for pushing
 *************************************************************************/
static void
genPointerPush (iCode *ic)
{
  operand *left = IC_LEFT (ic);
  int yoff;

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (left, ic);

  wassertl (IC_RIGHT (ic), "IPUSH_VALUE_AT_ADDRESS without right operand");
  wassertl (IS_OP_LITERAL (IC_RIGHT (ic)), "IPUSH_VALUE_AT_ADDRESS with non-literal right operand");
  wassertl (!operandLitValue (IC_RIGHT(ic)), "IPUSH_VALUE_AT_ADDRESS with non-zero right operand");

  bool needpulla = false;
  bool needpullx = false;
  bool needpully = false;

  if(AOP_TYPE(left)==AOP_SOF)
    needpullx=storeRegTempIfSurv(m6502_reg_x);

  needpulla=storeRegTempIfSurv(m6502_reg_a);

  yoff = setupDPTR(left, 0, NULL, false);

  needpully=storeRegTempIfSurv(m6502_reg_y);

  int size = getSize (operandType (left)->next);
  while (size--)
    {
      m6502_loadRegFromConst (m6502_reg_y, yoff+size);
      m6502_emitOp ("lda", INDFMT_IY, "DPTR");
      m6502_pushReg (m6502_reg_a, true);
    }

  m6502_loadOrFreeRegTemp(m6502_reg_y, needpully);
  m6502_loadOrFreeRegTemp(m6502_reg_a, needpulla);
  m6502_loadOrFreeRegTemp(m6502_reg_x, needpullx);
  m6502_freeAsmop (left, NULL);
}


/**************************************************************************
 * genSend - gen code for SEND
 *************************************************************************/
static void
genSend (set *sendSet)
{
  iCode *send1;
  iCode *send2;

  m6502_emitComment (TRACEGEN, "  %s", __func__);

  /* case 1: single parameter in A
   * case 2: single parameter in XA
   * case 3: first parameter in A, second parameter in X
   */
  send1 = setFirstItem (sendSet);
  send2 = setNextItem (sendSet);

  if (!send2)
    {
      int size;
      /* case 1 or 2, this is fairly easy */
      m6502_aopOp (IC_LEFT (send1), send1);
      size = AOP_SIZE (IC_LEFT (send1));
      wassert (size <= 2);
      if (size == 1)
        {
          m6502_loadRegFromAop (m6502_reg_a, AOP (IC_LEFT (send1)), 0);
        }
      else if (isOperandVolatile (IC_LEFT (send1), false))
        {
          /* use lsb to msb order for volatile operands */
          m6502_loadRegFromAop (m6502_reg_a, AOP (IC_LEFT (send1)), 0);
          m6502_loadRegFromAop (m6502_reg_x, AOP (IC_LEFT (send1)), 1);
        }
      else
        {
	  m6502_loadRegFromAop (m6502_reg_xa, AOP (IC_LEFT (send1)), 0);      
        }
      m6502_freeAsmop (IC_LEFT (send1), NULL);
    }
  else
    {
      /* case 3 */
      /* make sure send1 is the first argument and swap with send2 if not */
      if (send1->argreg > send2->argreg)
        {
          iCode * sic = send1;
          send1 = send2;
          send2 = sic;
        }
      m6502_aopOp (IC_LEFT (send1), send1);
      m6502_aopOp (IC_LEFT (send2), send2);
      if (IS_AOP_A (AOP (IC_LEFT (send2))))
        {
	  m6502_loadRegFromAop (m6502_reg_x, AOP (IC_LEFT (send2)), 0);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (IC_LEFT (send1)), 0);
	}
      else
        {
          m6502_loadRegFromAop (m6502_reg_a, AOP (IC_LEFT (send1)), 0);
          m6502_loadRegFromAop (m6502_reg_x, AOP (IC_LEFT (send2)), 0);
        }
      m6502_freeAsmop (IC_LEFT (send2), NULL);
      m6502_freeAsmop (IC_LEFT (send1), NULL);
    }
}

/**************************************************************************
 * genCall - generates a call statement
 *************************************************************************/
static void
genCall (iCode * ic)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  sym_link *dtype = operandType (left);
  sym_link *etype = getSpec (dtype);
  //  bool restoreBank = false;
  //  bool swapBanks = false;

  m6502_emitComment (TRACEGEN, "%s - reent:%d",
		     __func__, IFFUNC_ISREENT(dtype));
  m6502_printIC (ic);


  /* if caller saves & we have not saved then */
  if (!ic->regsSaved)
    saveRegisters (ic);

  /* if send set is not empty then assign */
  if (_S.sendSet && !regalloc_dry_run)
    {
      if (IFFUNC_ISREENT (dtype))
	{
	  /* need to reverse the send set */
	  genSend (reverseSet (_S.sendSet));
	}
      else
        {
	  genSend (_S.sendSet);
	}
      _S.sendSet = NULL;
    }

  /* make the call */
  if (IS_LITERAL (etype))
    {
      m6502_emitOp ("jsr", "0x%04X", ulFromVal (OP_VALUE (left)));
    }
  else
    {
      bool jump = (!ic->parmBytes && IFFUNC_ISNORETURN (OP_SYMBOL (left)->type));

      m6502_emitOp (jump ? "jmp" : "jsr", "%s", (OP_SYMBOL (left)->rname[0] ?
						 OP_SYMBOL (left)->rname : OP_SYMBOL (left)->name));
    }

  m6502_dirtyAllRegs ();

  _S.DPTRAttr[0].isLiteral=0;
  _S.DPTRAttr[1].isLiteral=0;
  _S.DPTRAttr[0].aop=NULL;
  _S.DPTRAttr[1].aop=NULL;
  _S.lastflag=-1;
  _S.carryValid=0;

  /* do we need to recompute the base ptr? */
  if (_S.funcHasBasePtr)
    saveBasePtr();

  /* if we need assign a result value */
  if ((IS_ITEMP (result) &&
       (OP_SYMBOL (result)->nRegs || OP_SYMBOL (result)->spildir)) || IS_TRUE_SYMOP (result))
    {
      m6502_useReg (m6502_reg_a);
      if (operandSize (result) > 1)
	m6502_useReg (m6502_reg_x);
      m6502_aopOp (result, ic);

      assignResultValue (result);

      m6502_freeAsmop (result, NULL);
    }

  /* adjust the stack for parameters if required */
  if (ic->parmBytes)
    pullNull (ic->parmBytes);

  /* if we had saved some registers then unsave them */
  if (ic->regsSaved && !IFFUNC_CALLEESAVES (dtype))
    unsaveRegisters (ic);
}

/**************************************************************************
 * genPcall - generates a call by pointer statement
 *************************************************************************/
static void
genPcall (iCode * ic)
{
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  sym_link *dtype;
  sym_link *etype;
  iCode * sendic;

  m6502_emitComment (TRACEGEN, __func__);
  m6502_printIC(ic);

  dtype = operandType (left)->next;
  etype = getSpec (dtype);

  /* if caller saves & we have not saved then */
  if (!ic->regsSaved)
    saveRegisters (ic);

  /* Go through the send set and mark any registers used by iTemps as */
  /* in use so we don't clobber them while setting up the return address */
  for (sendic = setFirstItem (_S.sendSet); sendic; sendic = setNextItem (_S.sendSet))
    {
      updateiTempRegisterUse (IC_LEFT (sendic));
    }

  // TODO: handle DIR/EXT with jmp [aa] or jmp [aaaa]

  if (!IS_LITERAL (etype))
    {
      m6502_updateCFA ();
      /* compute the function address */
      // put address in DPTR
      storeOperToDPTR (left, FARPTRSIZE, ic); // -1 is baked into initialization
    }

  /* if send set is not empty then assign */
  if (_S.sendSet && !regalloc_dry_run)
    {
      genSend (reverseSet (_S.sendSet));
      _S.sendSet = NULL;
    }

  /* make the call */
  if (!IS_LITERAL (etype))
    {
      m6502_emitOp("jsr","__sdcc_indirect_jsr");
      m6502_updateCFA ();
    }
  else
    {
      m6502_emitOp ("jsr", "0x%04X", ulFromVal (OP_VALUE (left)));
    }

  m6502_dirtyAllRegs ();

  _S.DPTRAttr[0].isLiteral=0;
  _S.DPTRAttr[1].isLiteral=0;
  _S.DPTRAttr[0].aop=NULL;
  _S.DPTRAttr[1].aop=NULL;
  _S.lastflag=-1;
  _S.carryValid=0;

  /* do we need to recompute the base ptr? */
  if (_S.funcHasBasePtr)
    saveBasePtr();

  /* if we need assign a result value */
  if ((IS_ITEMP (result) &&
       (OP_SYMBOL (result)->nRegs || OP_SYMBOL (result)->spildir)) || IS_TRUE_SYMOP (result))
    {
      m6502_useReg (m6502_reg_a);
      if (operandSize (result) > 1)
	m6502_useReg (m6502_reg_x);
      m6502_aopOp (result, ic);

      assignResultValue (result);

      m6502_freeAsmop (result, NULL);
    }

  /* adjust the stack for parameters if required */
  if (ic->parmBytes)
    pullNull (ic->parmBytes);

  /* if we had saved some registers then unsave them */
  if (ic->regsSaved && !IFFUNC_CALLEESAVES (dtype))
    unsaveRegisters (ic);
}

/**************************************************************************
 * resultRemat - result  is rematerializable
 *************************************************************************/
static int
resultRemat (iCode * ic)
{
  operand *result = IC_RESULT (ic);

  if (SKIP_IC (ic) || ic->op == IFX)
    return 0;

  if (result && IS_ITEMP (result))
    {
      symbol *sym = OP_SYMBOL (result);
      if (sym->remat && !POINTER_SET (ic))
	return 1;
    }

  return 0;
}

/**************************************************************************
 * inExcludeList - return 1 if the string is in exclude Reg list
 *************************************************************************/
static int
regsCmp (void *p1, void *p2)
{
  return (STRCASECMP ((char *) p1, (char *) (p2)) == 0);
}

static bool inExcludeList (char *s)
{
  const char *p = setFirstItem (options.excludeRegsSet);

  if (p == NULL || STRCASECMP (p, "none") == 0)
    return false;


  return isinSetWith (options.excludeRegsSet, s, regsCmp);
}

/**************************************************************************
 * genFunction - generated code for function entry
 *************************************************************************/
static void
genFunction (iCode * ic)
{
  symbol *sym = OP_SYMBOL (IC_LEFT (ic));
  sym_link *ftype;
  iCode *ric = (ic->next && ic->next->op == RECEIVE) ? ic->next : NULL;
  int stackAdjust = sym->stack;
  int recvSize = 0;

  /* create the function header */
  m6502_emitComment (ALWAYS, "-----------------------------------------");
  m6502_emitComment (ALWAYS, " function %s", sym->name);
  m6502_emitComment (ALWAYS, "-----------------------------------------");
  m6502_emitComment (ALWAYS, m6502_assignment_optimal ? "Register assignment is optimal." : "Register assignment might be sub-optimal.");
  m6502_emitComment (ALWAYS, "Stack space usage: %d bytes.", sym->stack);

  if(ric)
    {
      symbol *p1 = NULL, *p2 = NULL;

      p1 = OP_SYMBOL (IC_RESULT (ric));
      recvSize += p1 ? getSize (p1->type) : 0;
      iCode *ric2 = (ric->next && ric->next->op == RECEIVE) ? ric->next : NULL;
      if(recvSize==1 && ric2)
        {
          p2 = OP_SYMBOL (IC_RESULT (ric2));
          recvSize += p2 ? getSize (p2->type) : 0;
        }
    }

  m6502_emitComment (ALWAYS, "%s - rcv size = %d", __func__, recvSize);

  emitcode ("", "%s:", sym->rname);
  genLine.lineCurr->isLabel = 1;
  ftype = operandType (IC_LEFT (ic));

  if (recvSize==1 || recvSize == 2)
    {
      m6502_useReg (m6502_reg_a);
      m6502_reg_a->isDead=0;
    }
  if (recvSize==2)
    {
      m6502_useReg (m6502_reg_x);
      m6502_reg_x->isDead=0;
    }

  _S.stackBase = 0;
  _S.stackPushes = 0;
  m6502_reg_x->stackOffset = 0;
  _S.lastflag=-1;
  _S.carryValid=0;

  if (options.debug && !regalloc_dry_run)
    debugFile->writeFrameAddress (NULL, m6502_reg_sp, 0);

  if (IFFUNC_ISNAKED (ftype))
    {
      m6502_emitComment (ALWAYS, "naked function: no prologue.");
      return;
    }

  /* if this is an interrupt service routine then
     save a, x & y  */
  if (IFFUNC_ISISR (sym->type))
    {
      if (!inExcludeList ("a"))
        m6502_pushReg (m6502_reg_a, true);
      if (!inExcludeList ("x"))
        m6502_pushReg (m6502_reg_x, true);
      if (!inExcludeList ("y"))
        m6502_pushReg (m6502_reg_y, true);
    }

  /* For some cases it is worthwhile to perform a RECEIVE iCode */
  /* before setting up the stack frame completely. */
  int numStackParams = 0;
  while (ric && ric->next && ric->next->op == RECEIVE)
    ric = ric->next;
  while (ric && IC_RESULT (ric))
    {
      symbol *rsym = OP_SYMBOL (IC_RESULT (ric));
      int rsymSize = rsym ? getSize (rsym->type) : 0;

      if (rsym->isitmp)
        {
          if (rsym && rsym->regType == REG_CND)
            rsym = NULL;
          if (rsym && (/*rsym->accuse ||*/ rsym->ruonly))
            rsym = NULL;
          if (rsym && (rsym->isspilt || rsym->nRegs == 0) && rsym->usl.spillLoc)
            rsym = rsym->usl.spillLoc;
        }

      /* If the RECEIVE operand immediately spills to the first entry on the  */
      /* stack, we can push it directly rather than use an sp relative store. */
      if (rsym && rsym->onStack && rsym->stack == -_S.stackPushes - rsymSize)
        {
          int ofs;

          genLine.lineElement.ic = ric;
          m6502_emitComment (TRACEGEN, "genReceive: size=%d", rsymSize);
          //          for (ofs = 0; ofs < rsymSize; ofs++)
	  m6502_useReg (m6502_reg_a);
          for (ofs = rsymSize-1; ofs >=0; ofs--)
            {
              reg_info *reg = m6502_aop_pass[ofs + (ric->argreg - 1)]->aopu.aop_reg[0];
              m6502_emitComment (TRACEGEN, "pushreg: ofs=%d", ofs);
              m6502_pushReg (reg, true);
              //              if (reg->rIdx == A_IDX)
              //                accIsFree = 1;
              stackAdjust--;
            }
          genLine.lineElement.ic = ic;
          ric->generated = 1;
        }
      ric = (ric->prev && ric->prev->op == RECEIVE) ? ric->prev : NULL;
    }

  /* adjust the stack for the function */
  if (stackAdjust)
    m6502_adjustStack (-stackAdjust);

  _S.stackBase = sym->stack;
  _S.stackPushes = 0;
  m6502_reg_x->stackOffset = 0;
  _S.funcHasBasePtr = 0;

  if ( stackAdjust || sym->stack || numStackParams || IFFUNC_ISREENT(sym->type) )
    {
      saveBasePtr();
      _S.funcHasBasePtr = 1;
    }

  /* if critical function then turn interrupts off */
  if (IFFUNC_ISCRITICAL (ftype))
    {
      m6502_emitOp ("php", "");
      m6502_emitOp ("sei", "");
    }
}

/**************************************************************************
 * genEndFunction - generates epilogue for functions
 *************************************************************************/
static void
genEndFunction (iCode * ic)
{
  symbol *sym = OP_SYMBOL (IC_LEFT (ic));

  m6502_emitComment (TRACEGEN, __func__);
  m6502_emitComment (REGOPS, "  %s %s", __func__, m6502_regInfoStr() );

  if (IFFUNC_ISNAKED (sym->type))
    {
      m6502_emitComment (ALWAYS, "naked function: no epilogue.");
      if (options.debug && currFunc && !regalloc_dry_run)
        debugFile->writeEndFunction (currFunc, ic, 0);
      return;
    }

  if (IFFUNC_ISCRITICAL (sym->type))
    m6502_emitOp ("plp", "");

  if (IFFUNC_ISREENT (sym->type) || options.stackAuto)
    {
    }

  if (_S.funcHasBasePtr)
    restoreBasePtr();

  if (_S.stackPushes)
    emitcode("ERROR","_S.stackPushes=%d in genEndFunction", _S.stackPushes);

  if (sym->stack)
    {
      //  if (sym->regsUsed && sym->regsUsed->size)
      // FIXME: need to figure out how to get the exact registers needed by the function return
      if(IS_VOID(sym->type->next))
        {
          m6502_freeReg(m6502_reg_a);
          m6502_freeReg(m6502_reg_x);
          m6502_freeReg(m6502_reg_y);
        }
      else
        {
          m6502_useReg(m6502_reg_a);
          m6502_useReg(m6502_reg_x);
        }
      _S.stackPushes += sym->stack;
      m6502_adjustStack (sym->stack);
    }

  if (IFFUNC_ISISR (sym->type))
    {
      if (!inExcludeList ("y"))
        m6502_pullReg (m6502_reg_y);
      if (!inExcludeList ("x"))
        m6502_pullReg (m6502_reg_x);
      if (!inExcludeList ("a"))
        m6502_pullReg (m6502_reg_a);

      /* if debug then send end of function */
      if (options.debug && currFunc && !regalloc_dry_run)
	debugFile->writeEndFunction (currFunc, ic, 1);

      m6502_emitOp ("rti", "");
    }
  else
    {
      if (IFFUNC_CALLEESAVES (sym->type))
        {
          int i;

          /* if any registers used */
          if (sym->regsUsed)
            {
              /* save the registers used */
              for (i = sym->regsUsed->size; i >= 0; i--)
                {
                  if (bitVectBitValue (sym->regsUsed, i) || (m6502_ptrRegReq && (i == XY_IDX)))
                    {
                      // FIXME
                      emitcode ("pop", "%s", m6502_regWithIdx (i)->name); /* Todo: Cost. Can't find this instruction in manual! */
                    }
                }
            }
        }

      /* if debug then send end of function */
      if (options.debug && currFunc && !regalloc_dry_run)
	debugFile->writeEndFunction (currFunc, ic, 1);

      m6502_emitOp ("rts", "");
    }
}

/**************************************************************************
 * genRet - generate code for return statement
 *************************************************************************/
static void genRet (iCode * ic)
{
  operand *left   = IC_LEFT (ic);

  int size, offset = 0;
  //  int pushed = 0;
  bool delayed_x = false;

  m6502_emitComment (TRACEGEN, __func__);
  m6502_emitComment (REGOPS, "  %s %s", __func__, m6502_regInfoStr() );


  /* if we have no return value then
     just generate the "ret" */
  if (!left)
    goto jumpret;

  /* we have something to return then
     move the return value into place */
  m6502_aopOp (left, ic);

  size = AOP_SIZE (left);
  const bool bigreturn = IS_STRUCT (operandType (left));

  if (bigreturn)
    {
      // FIXME: only up to size 8 is supported
      if(size>8)
        {
          if (!regalloc_dry_run)
            emitcode ("ERROR","  %s: return size>8 not supported", __func__);
          goto jumpret;
        }

      for(offset=size-1; offset>=2; offset--)
        m6502_transferAopAop (AOP (left), offset, m6502_aop_pass[offset], 0);

      m6502_loadRegFromAop (m6502_reg_xa, AOP (left), 0);
      goto jumpret;
    }

  if (AOP_TYPE (left) == AOP_LIT)
    {
      /* If returning a literal, we can load the bytes of the return value */
      /* in any order. */
      for(offset=0; offset<size; offset++)
        {
	  m6502_transferAopAop (AOP (left), offset, m6502_aop_pass[offset], 0);
	}
    }
  else
    {
      /* Take care when swapping a and x */
      if (AOP_TYPE (left) == AOP_REG && size > 1 && AOP (left)->aopu.aop_reg[0]->rIdx == X_IDX)
        {
	  delayed_x = true;
	  storeRegTemp (m6502_reg_x, true);
	}

      offset = size - 1;
      if(size>1 && AOP_TYPE(left)==AOP_SOF)
	{
	  int i;
	  for (i=size-1; i>1; i--)
	    {
	      m6502_loadRegFromAop (m6502_reg_a, AOP(left), i);
	      m6502_storeRegToAop (m6502_reg_a, m6502_aop_pass[i], 0);
	    }
	  m6502_loadRegFromAop (m6502_reg_a, AOP(left), 1);
	  storeRegTemp (m6502_reg_a, true);
	  m6502_loadRegFromAop (m6502_reg_a, AOP(left), 0);
	  m6502_loadRegTemp (m6502_reg_x);
	}
      else
	{
	  while (size--)
	    {
	      if (!(delayed_x && !offset))
		m6502_transferAopAop (AOP (left), offset, m6502_aop_pass[offset], 0);
	      offset--;
	    }
	}

      if (delayed_x)
	m6502_loadRegTemp (m6502_reg_a);
    }

  m6502_freeAsmop (left, NULL);

 jumpret:
  /* generate a jump to the return label
     if the next is not the return statement */
  if (!(ic->next && ic->next->op == LABEL && IC_LABEL (ic->next) == returnLabel))
    m6502_emitOp ("jmp", "%05d$", m6502_safeLabelNum (returnLabel));

}

/**************************************************************************
 * genLabel - generates a label
 *************************************************************************/
static void genLabel (iCode * ic)
{
  int i;

  m6502_emitComment (TRACEGEN, __func__);
  m6502_emitComment (REGOPS, "  %s %s", __func__, m6502_regInfoStr() );


  for(i=0;i<NUM_TEMP_REGS;i++)
    m6502_dirtyRegTemp (i);

  _S.DPTRAttr[0].isLiteral=0;
  _S.DPTRAttr[1].isLiteral=0;
  _S.DPTRAttr[0].aop=NULL;
  _S.DPTRAttr[1].aop=NULL;

  _S.lastflag=-1;
  _S.carryValid=0;

  /* special case never generate */
  if (IC_LABEL (ic) == entryLabel)
    return;

  /* For the high level labels we cannot depend on any */
  /* register's contents. Amnesia time.                */
  m6502_dirtyAllRegs();

  if (options.debug && !regalloc_dry_run)
    debugFile->writeLabel (IC_LABEL (ic), ic);

  m6502_safeEmitLabel (IC_LABEL (ic));
}

/**************************************************************************
 * genGoto - generates a jmp
 *************************************************************************/
static void genGoto (iCode * ic)
{
  m6502_emitComment (TRACEGEN, __func__);
  m6502_emitOp ("jmp", "%05d$", m6502_safeLabelNum (IC_LABEL (ic)));
}

/**************************************************************************
 * genMult - generates code for multiplication
 *************************************************************************/
static void
genMult (iCode * ic)
{
  /* Shouldn't occur - all done through function calls */
  wassertl (0, "Multiplication is handled through support function calls");
}

/**************************************************************************
 * genDiv - generates code for division
 *************************************************************************/
static void
genDiv (iCode * ic)
{
  /* Shouldn't occur - all done through function calls */
  wassertl (0, "Division is handled through support function calls");
}

/**************************************************************************
 * genMod - generates code for division
 *************************************************************************/
static void
genMod (iCode * ic)
{
  /* Shouldn't occur - all done through function calls */
  wassertl (0, "Division is handled through support function calls");
}

static const struct binst_t binst[5] = {
  { "z", "beq", "bne" },
  { "c", "bcc", "bcs" },
  { "n", "bpl", "bmi" },
  { "v", "bvc", "bvs" },
  { "a", "bra", "brn" }
};

static const char *
negateBranch(const char *inst)
{
  for(int i=0; i<sizeof(binst)/sizeof(struct binst_t); i++)
    {
      if (!strcmp (inst, binst[i].set))
        return(binst[i].clear);
      if (!strcmp (inst, binst[i].clear))
        return(binst[i].set);
    }
  emitcode("ERROR", "%s : unknown inst %s", __func__, inst );

  return "ERROR";
}

static const char *
findBranch(const char *jval)
{
  for(int i=0; i<sizeof(binst)/sizeof(struct binst_t); i++)
    {
      if (!strcmp (jval, binst[i].flag))
        return(binst[i].set);
    }
  emitcode("ERROR", "%s : unknown jval %s", __func__, jval );

  return "ERROR";
}

/**************************************************************************
 * m6502_genIfxJump :- will create a jump depending on the ifx
 *************************************************************************/
void
m6502_genIfxJump (iCode * ic, char *jval)
{
  symbol *jlbl;
  symbol *skip_lbl = m6502_safeNewiTempLabel (NULL);
  const char *inst;
  bool negate=false;

  m6502_emitComment (TRACEGEN, "%s : %s", __func__, jval);

  if(jval[0]=='~')
    {
      jval++;
      negate=true;
    }

  inst = findBranch(jval);

  if (IC_TRUE (ic))
    {
      /* if true label then we jump if condition
	 supplied is true */
      jlbl = IC_TRUE (ic);
    }
  else
    {
      /* false label is present */
      jlbl = IC_FALSE (ic);
      inst = negateBranch(inst);
    }

  if(negate)
    inst = negateBranch(inst);

  if(!strcmp (inst, "bra") )
    {
      m6502_emitBranch ("jmp", jlbl);
    }
  else if(!strcmp (inst, "brn") )
    {
      // do nothing
    }
  else
    {
      m6502_emitBranch (inst, skip_lbl);
      m6502_emitBranch ("jmp", jlbl);
      m6502_safeEmitLabel (skip_lbl);
    }

  /* mark the icode as generated */
  ic->generated = 1;
}


/**************************************************************************
 * exchangedCmp : returns the opcode need if the two operands are
 *                exchanged in a comparison
 *************************************************************************/
static int
exchangedCmp (int opcode)
{
  switch (opcode)
    {
    case '<':
      return '>';
    case '>':
      return '<';
    case LE_OP:
      return GE_OP;
    case GE_OP:
      return LE_OP;
    case NE_OP:
      return NE_OP;
    case EQ_OP:
      return EQ_OP;
    default:
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "opcode not a comparison");
    }
  return EQ_OP;                 /* shouldn't happen, but need to return something */
}

/**************************************************************************
 * negatedCmp : returns the equivalent opcode for when a comparison
 *              if not true
 *************************************************************************/
static int
negatedCmp (int opcode)
{
  switch (opcode)
    {
    case '<':
      return GE_OP;
    case '>':
      return LE_OP;
    case LE_OP:
      return '>';
    case GE_OP:
      return '<';
    case NE_OP:
      return EQ_OP;
    case EQ_OP:
      return NE_OP;
    default:
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "opcode not a comparison");
    }
  return EQ_OP;                 /* shouldn't happen, but need to return something */
}

/**************************************************************************
 * nameCmp : helper function for human readable debug output
 *************************************************************************/
static char *
nameCmp (int opcode)
{
  switch (opcode)
    {
    case '<':
      return "<";
    case '>':
      return ">";
    case LE_OP:
      return "<=";
    case GE_OP:
      return ">=";
    case NE_OP:
      return "!=";
    case EQ_OP:
      return "==";
    default:
      return "invalid";
    }
}

/**************************************************************************
 * branchInstCmp : returns the conditional branch instruction that
 *                 will branch if the comparison is true
 *************************************************************************/
static char *
branchInstCmp (int opcode, int sign)
{
  switch (opcode)
    {
    case '<':
      if (sign)
	return "blt";
      else
	return "bcc";
    case '>':
      if (sign)
	return "bgt";
      else
	return "bhi";
    case LE_OP:
      if (sign)
	return "ble";
      else
	return "bls";
    case GE_OP:
      if (sign)
	return "bge";
      else
	return "bcs";
    case NE_OP:
      return "bne";
    case EQ_OP:
      return "beq";
    default:
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__, "opcode not a comparison");
    }
  return "brn";
}


/**************************************************************************
 * genCmp :- greater or less than (and maybe with equal) comparison
 *************************************************************************/
static void
genCmp (iCode * ic, iCode * ifx)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int size, opcode;
  int sign = 0, offset = 0;
  unsigned long long lit = 0ull;
  char *cmp_inst = NULL;
  char *br_inst = NULL;
  symbol *ifx_jmp_lbl = NULL;
  symbol *true_lbl = NULL;
  symbol *false_lbl = NULL;
  bool needloada = false;

  opcode = ic->op;

  // TODO: don't use signed when unsigned will do
  if (IS_SPEC (operandType (left)) && IS_SPEC (operandType (right)))
    {
      sym_link *letype, *retype;
      letype = getSpec (operandType (left));
      retype = getSpec (operandType (right));
      sign = !(SPEC_USIGN (letype) | SPEC_USIGN (retype));
    }

  /* assign the amsops */
  m6502_aopOp (left, ic);
  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);
  m6502_printIC (ic);

  size = max (AOP_SIZE (left), AOP_SIZE (right));

  m6502_emitComment (TRACEGEN, "%s (%s, size:%d, sign:%d, ifx:%d)",
		     __func__, nameCmp (opcode), size, sign, ifx?1:0 );

  /* need register operand on left, prefer literal operand on right */
  if ((AOP_TYPE (right) == AOP_REG) || AOP_TYPE (left) == AOP_LIT)
    {
      // don't swap if left is A
      // FIXME: not sure if this is necessary
      if(!((AOP_TYPE (left) == AOP_REG) && AOP (left)->aopu.aop_reg[0]->rIdx == A_IDX))
        {
          operand *temp = left;
          left = right;
          right = temp;
          opcode = exchangedCmp (opcode);
          m6502_emitComment (TRACEGEN|VVDBG, "  %s (exchanged opcode to %s)", __func__,
			     nameCmp (opcode));

        }
    }

  // TODO: special case for compare with 0
  // TODO: also special case >xxff
  // TODO: reg > imm (currently implemented with imm - reg) try to implement with reg - imm
  // <xx00 is already optimized

  bool right_zero = (AOP_TYPE (right) == AOP_LIT) && (ullFromVal(AOP (right)->aopu.aop_lit) == 0 );
  bool result_in_a = false;

  if(right_zero)
    m6502_emitComment (TRACEGEN|VVDBG, "%s - right is zero", 
		       __func__);

  if (ifx)
    {
      if (IC_TRUE (ifx))
        {
          ifx_jmp_lbl = IC_TRUE (ifx);
          opcode = negatedCmp (opcode);
        }
      else
        {
          /* false label is present */
          ifx_jmp_lbl = IC_FALSE (ifx);
        }
    }
  else
    {
      true_lbl = m6502_safeNewiTempLabel (NULL);
    }

  if (sign && right_zero && opcode=='<')
    br_inst = "bmi";
  else if(sign && right_zero && opcode==GE_OP)
    br_inst = "bpl";

  if (sign && right_zero && m6502_aopCanBit(AOP(left))
      && (opcode=='<' || opcode ==GE_OP) )
    {
      m6502_accopWithAop ("bit", AOP (left), size-1);
    }
  else if (sign && right_zero && AOP_TYPE(left)==AOP_REG
           && (opcode=='<' || opcode==GE_OP) ) 
    {
      m6502_emitCmp(AOP (left)->aopu.aop_reg[size-1], 0);
    }
  else if (sign && right_zero && opcode=='>' && AOP_TYPE(left)==AOP_REG) 
    {
      m6502_emitCmp(AOP (left)->aopu.aop_reg[size-1], 0);

      if(ifx)
        {
          symbol *skiplbl = m6502_safeNewiTempLabel (NULL);

          m6502_emitBranch ("bpl", skiplbl);
          m6502_emitBranch ("jmp", ifx_jmp_lbl);
          m6502_safeEmitLabel (skiplbl);
        }
      else
        {
          false_lbl = m6502_safeNewiTempLabel (NULL);
          m6502_emitBranch ("bmi", false_lbl); 
        }

      if(size==2)
        {
          if(ifx)
            m6502_emitBranch ("bne", ifx_jmp_lbl);
          else
            m6502_emitBranch ("bne", true_lbl);

          m6502_emitCmp(AOP (left)->aopu.aop_reg[0], 0);
        }
      br_inst = "bne";
    }
  else if (!sign && size == 1 && IS_AOP_X (AOP (left)) && isAddrSafe(right, m6502_reg_x) )
    {
      if(IS_AOP_A(AOP(result)))
	{
          m6502_loadRegFromConst (m6502_reg_a, 0);
          result_in_a=true;
	}
      m6502_accopWithAop ("cpx", AOP (right), 0);
    }
  else if (!sign && size == 1 && IS_AOP_Y (AOP (left)) && isAddrSafe(right, m6502_reg_y) )
    {
      if(IS_AOP_A(AOP(result)))
	{
          m6502_loadRegFromConst (m6502_reg_a, 0);
          result_in_a=true;
	}
      m6502_accopWithAop ("cpy", AOP (right), 0);
    }
  else if (!sign && size == 1 && IS_AOP_A (AOP (left)) )
    {
      m6502_accopWithAop ("cmp", AOP (right), 0);
      if (right_zero && opcode=='>' && IS_AOP_A(AOP(result)) )
        {
          result_in_a=true;
	}
    }
  else
    {
      // need V flag for signed compare
      // FIXME: is this covered above?
      if (!sign && size == 1)
        {
          cmp_inst = "cmp";
        }
      else
        {
          cmp_inst = "sub";

          /* These conditions depend on the Z flag bit, but Z is */
          /* only valid for the last byte of the comparison, not */
          /* the whole value. So exchange the operands to get a  */
          /* comparison that doesn't depend on Z. (This is safe  */
          /* to do here since ralloc won't assign multi-byte     */
          /* operands to registers for comparisons)              */
          if ((opcode == '>') || (opcode == LE_OP))
            {
              operand *temp = left;
              left = right;
              right = temp;
              opcode = exchangedCmp (opcode);
            }

          if ((AOP_TYPE (right) == AOP_LIT) && !isOperandVolatile (left, false))
            {
              lit = ullFromVal (AOP (right)->aopu.aop_lit);
              while ((offset<(size-1)) && (((lit >> (8 * offset)) & 0xff) == 0))
                {
                  offset++;
                }

              if(offset==(size-1) && (lit==0 || sign == 0))
                {
                  cmp_inst = "cmp";
                }
            }
        }

      needloada = storeRegTempIfSurv (m6502_reg_a);
      for( ; offset<size; offset++)
        {
          m6502_emitComment (TRACEGEN|VVDBG, "   %s - size = %d offset = %d",
			     __func__, size, offset);

          if (AOP_TYPE (right) == AOP_REG && AOP(right)->aopu.aop_reg[offset]->rIdx == A_IDX)
            {
              m6502_emitComment (TRACEGEN|VVDBG, "   GenCmp - REG_A");
	      if(!needloada)
		storeRegTemp (m6502_reg_a, true);

              m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
	      if (!strcmp (cmp_inst, "sub"))
		{
		  m6502_emitSetCarry (1);
		  m6502_emitRegTempOp("sbc", m6502_getLastTempOfs() );
		}
	      else
		{
		  m6502_emitRegTempOp(cmp_inst, m6502_getLastTempOfs() );
		}
	      if(!needloada) 
		m6502_loadRegTemp(NULL);
            }
          else
            {
              m6502_emitComment (TRACEGEN|VVDBG, "   %s - generic off:%d sub:%s",
				 __func__, offset, cmp_inst);

              if (!strcmp(cmp_inst, "sub"))
		{
                  m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
		  m6502_emitSetCarry (1);
		  m6502_accopWithAop ("sbc", AOP (right), offset);
		}
	      else
		{
                  if(!strcmp(cmp_inst, "cmp") && AOP_TYPE(left)==AOP_REG && AOP_TYPE(right)!=AOP_SOF )
                    {
                      if(AOP(left)->aopu.aop_reg[offset]->rIdx==X_IDX)
                        cmp_inst="cpx";
                      else if(AOP(left)->aopu.aop_reg[offset]->rIdx==Y_IDX)
                        cmp_inst="cpy";                
                    }
                  else
                    {
                      m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
                    }
		  m6502_accopWithAop (cmp_inst, AOP (right), offset);
		}
            }
          m6502_freeReg (m6502_reg_a);
          cmp_inst = "sbc";
        }
    }

  if(!sign && size == 1 && right_zero)
    br_inst = "beq";

  if(br_inst==NULL)
    br_inst = !result_in_a ? branchInstCmp (opcode, sign) : branchInstCmp (negatedCmp(opcode), sign);

  if (ifx)
    {
      symbol *skiplbl = m6502_safeNewiTempLabel (NULL);

      if(needloada)
        {
	  if (!strcmp(br_inst, "bcc") || !strcmp(br_inst, "bcs"))
	    m6502_loadRegTemp(m6502_reg_a);
	  else
            m6502_loadRegTempNoFlags (m6502_reg_a, needloada);
        }
      else
	m6502_freeReg (m6502_reg_a);

      m6502_emitBranch (br_inst, skiplbl);
      m6502_emitBranch ("jmp", ifx_jmp_lbl);
      m6502_safeEmitLabel (skiplbl);

      /* mark the icode as generated */
      ifx->generated = 1;
    }
  else if(result_in_a)
    {
      // reuse allocated label
      symbol *skiplbl = true_lbl;

      m6502_emitBranch (br_inst, skiplbl);
      m6502_loadRegFromConst (m6502_reg_a, 1);
      m6502_safeEmitLabel (skiplbl);
      m6502_dirtyReg (m6502_reg_a);
    }
  else
    {
      symbol *skiplbl = m6502_safeNewiTempLabel (NULL);

      if (!needloada)
        needloada = storeRegTempIfSurv (m6502_reg_a);

      m6502_emitBranch (br_inst, true_lbl);
      if (false_lbl)
        m6502_safeEmitLabel (false_lbl);

      m6502_loadRegFromConst (m6502_reg_a, 0);
      // FIXME: for 6502 change this to beq when optimizing for size
      m6502_emitBranch ("bra", skiplbl);
      m6502_safeEmitLabel (true_lbl);
      m6502_loadRegFromConst (m6502_reg_a, 1);
      m6502_safeEmitLabel (skiplbl);
      m6502_dirtyReg (m6502_reg_a);
      m6502_storeRegToFullAop (m6502_reg_a, AOP (result), false);
      m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
    }

  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (result, NULL);
}


/**************************************************************************
 * genCmpEQorNE - equal or not equal comparison
 *************************************************************************/
static void
genCmpEQorNE (iCode * ic, iCode * ifx)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);
  int opcode;
  int size;
  symbol *jlbl = NULL;
  symbol *tlbl_NE = NULL;
  symbol *tlbl_EQ = NULL;
  symbol *NE_label = NULL;
  symbol *end_label = NULL;

  bool needloada = false;
  bool needloadx = false;
  bool needloady = false;
  reg_info *reg = NULL;
  int offset = 0;
  bool early_result = false;
  bool result_equal = false;

  end_label = m6502_safeNewiTempLabel (NULL);

  opcode = ic->op;

  m6502_emitComment (TRACEGEN, "%s - (%s) ifx:%d", 
		     __func__, nameCmp (opcode), ifx?1:0);

  /* assign the amsops */
  m6502_aopOp (left, ic);
  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);
  m6502_printIC (ic);

  // TODO: rearrange the compare when one of the lit is zero
  // and the flag is already in the status register

  /* need register operand on left, prefer literal operand on right */
  if ((AOP_TYPE (right) == AOP_REG) || AOP_TYPE (left) == AOP_LIT)
    {
      // don't swap if left is A
      if(!((AOP_TYPE (left) == AOP_REG) && AOP (left)->aopu.aop_reg[0]->rIdx == A_IDX))
	{
	  operand *temp = left;
	  left = right;
	  right = temp;
	  opcode = exchangedCmp (opcode);
	}
    }

  size = max (AOP_SIZE (left), AOP_SIZE (right));

  if (ifx)
    {
      if (IC_TRUE (ifx))
	{
	  jlbl = IC_TRUE (ifx);
	  opcode = negatedCmp (opcode);
	}
      else
	{
	  /* false label is present */
	  jlbl = IC_FALSE (ifx);
	}
    }

  if(AOP_TYPE (left) == AOP_REG)
    m6502_emitComment (TRACEGEN|VVDBG, "   %s - left is reg: %s", __func__, AOP (left)->aopu.aop_reg[offset]->name);
  else
    m6502_emitComment (TRACEGEN|VVDBG, "   %s - left is not not a reg", __func__);

  result_equal = (opcode == EQ_OP)?1:0;

  if (!ifx)
    {
      NE_label = m6502_safeNewiTempLabel (NULL);
      needloada = storeRegTempIfSurv(m6502_reg_a);

      if(IS_AOP_WITH_A(AOP(left)) || AOP_TYPE(left)==AOP_SOF || AOP_TYPE(right)==AOP_SOF)
        {
          reg=m6502_reg_a;

          if(AOP_TYPE(left)==AOP_SOF || AOP_TYPE(right)==AOP_SOF)
            needloadx=storeRegTempIfSurv(m6502_reg_x);
        }
      else// if(AOP_TYPE(left)!=AOP_REG)
        {
	  if(AOP_TYPE(left)!=AOP_REG)
	    {
	      reg=m6502_getFreeIdxReg();

	      if(!reg)
		{
		  reg=m6502_reg_y;
		  needloady=storeRegTempIfSurv(m6502_reg_y);
		}
	    }
          m6502_loadRegFromConst (m6502_reg_a, !result_equal);
          early_result=true;
        }

      if(AOP_TYPE(right)==AOP_LIT && (ullFromVal (AOP(right)->aopu.aop_lit))==0)
        m6502_emitComment (TRACEGEN|VVDBG, "    %s - cmp with zero", __func__);
   
      for(offset=0; offset<size; offset++)
        {
	  if (AOP_TYPE (left) == AOP_REG 
              && (AOP (left)->aopu.aop_reg[offset]==m6502_reg_a || AOP_TYPE(right)!=AOP_SOF) )
            m6502_accopWithAop (m6502_cmp[AOP (left)->aopu.aop_reg[offset]->rIdx], AOP (right), offset);
          else
	    {
              m6502_emitComment (TRACEGEN|VVDBG, "    %s - not a reg", __func__);

              m6502_loadRegFromAop (reg, AOP (left), offset);
              if(AOP_TYPE(right)==AOP_LIT)
                m6502_emitCmp(reg, (ullFromVal (AOP(right)->aopu.aop_lit))>>(offset*8) );
              else
	        m6502_accopWithAop (m6502_cmp[reg->rIdx], AOP (right), offset);
	    }

	  if(!early_result)
	    m6502_emitBranch ("bne", NE_label);
	  else
	    m6502_emitBranch ("bne", end_label);
	}

      // set EQ
      m6502_loadRegFromConst (m6502_reg_a, result_equal);

      if(!early_result)
        {
          // jmp to end
          m6502_emitBranch ("bra", end_label);
	  m6502_safeEmitLabel (NE_label);
          // set ne
          m6502_loadRegFromConst (m6502_reg_a, !result_equal);
        }

      m6502_safeEmitLabel (end_label);

      m6502_dirtyReg (m6502_reg_a);
      if(reg)
        m6502_dirtyReg (reg);

      m6502_storeRegToFullAop (m6502_reg_a, AOP (result), true);
      m6502_loadOrFreeRegTemp (m6502_reg_y, needloady);
      m6502_loadOrFreeRegTemp (m6502_reg_x, needloadx);
      m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
    }
  else
    {
      if(size==1 
	 && AOP_TYPE(right)==AOP_LIT && AOP_TYPE(left)!=AOP_SOF)
	{
	  bool restore_a=false;
	  reg_info *reg = NULL;

	  if(AOP_TYPE(left)==AOP_REG)
	    {
	      reg=AOP(left)->aopu.aop_reg[0];
	    }
	  else
	    {
	      reg=m6502_getFreeByteReg();
	      if(!reg)
		{
		  reg=m6502_reg_a;
		  restore_a=storeRegTempIfSurv(m6502_reg_a);
		}
	      m6502_loadRegFromAop (reg, AOP(left), 0);
	    }
	  m6502_emitCmp(reg, ullFromVal (AOP(right)->aopu.aop_lit));
	  if(restore_a)
	    m6502_loadRegTempNoFlags(m6502_reg_a, true);
	}
      else
	// TODO: could clobber A if reg = XA?
	{
          for(offset=0; offset<size; offset++)
	    {
	      if (AOP_TYPE (left) == AOP_REG 
		  && (AOP (left)->aopu.aop_reg[offset]==m6502_reg_a || AOP_TYPE(right)!=AOP_SOF) )
		m6502_accopWithAop (m6502_cmp[AOP (left)->aopu.aop_reg[offset]->rIdx], AOP (right), offset);
	      else 
		{
                  m6502_emitComment (TRACEGEN|VVDBG, "    %s - not a reg", __func__);

		  // TODO? why do we push when we could cpx?
		  if (!(AOP_TYPE (left) == AOP_REG && AOP (left)->aopu.aop_reg[offset]->rIdx == A_IDX))
		    {
		      if(AOP_TYPE(right) == AOP_REG)
			{
			  m6502_emitComment (TRACEGEN|VVDBG, "   genCmpEQorNE right is reg: %s",AOP (right)->aopu.aop_reg[offset]->name);
			}

		      // FIXME: always?
		      //                  storeRegTemp (m6502_reg_a, true);
		      //                  needloada = true;
		      needloada = storeRegTempIfSurv(m6502_reg_a);
		      //m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
		    }
		  if(m6502_reg_x->aop&&m6502_sameRegs(m6502_reg_x->aop, AOP(left))&&m6502_reg_x->aopofs==offset)
		    {
		      m6502_accopWithAop ("cpx", AOP (right), offset);
		    }
		  else
		    {
		      m6502_loadRegFromAop (m6502_reg_a, AOP (left), offset);
		      m6502_accopWithAop ("cmp", AOP (right), offset);
		    }
		  m6502_loadRegTempNoFlags (m6502_reg_a, needloada);
		  needloada = false;
		}
	      if (offset<size-1)
		{
		  if (!tlbl_NE)
		    tlbl_NE = m6502_safeNewiTempLabel (NULL);

		  m6502_emitBranch ("bne", tlbl_NE);
		}
	    }
	}

      if (opcode == EQ_OP)
	{
	  if (!tlbl_EQ)
	    tlbl_EQ = m6502_safeNewiTempLabel (NULL);

	  m6502_emitBranch ("beq", tlbl_EQ);
	  if (tlbl_NE)
	    m6502_safeEmitLabel (tlbl_NE);

	  m6502_emitBranch ("jmp", jlbl);
	  m6502_safeEmitLabel (tlbl_EQ);
	}
      else
	{
	  if (!tlbl_NE)
	    tlbl_NE = m6502_safeNewiTempLabel (NULL);

	  m6502_emitBranch ("bne", tlbl_NE);
	  m6502_emitBranch ("jmp", jlbl);
	  m6502_safeEmitLabel (tlbl_NE);
	}

      /* mark the icode as generated */
      ifx->generated = 1;
    }

  m6502_dirtyReg (m6502_reg_a);  // FIXME: find a less conservative place for this
  _S.DPTRAttr[0].aop=NULL;
  _S.DPTRAttr[1].aop=NULL;

  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * genAndOp - for && operation
 *************************************************************************/
static void genAndOp (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  symbol *tlbl, *tlbl0;
  bool needpulla;

  m6502_emitComment (TRACEGEN, __func__);

  // TODO: optimize & 0xff as cast when signed

  /* note here that && operations that are in an
     if statement are taken away by backPatchLabels
     only those used in arthmetic operations remain */
  m6502_aopOp (left, ic);
  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);
  m6502_printIC (ic);

  tlbl = m6502_safeNewiTempLabel (NULL);
  tlbl0 = m6502_safeNewiTempLabel (NULL);

  needpulla = pushRegIfSurv (m6502_reg_a);
  asmopToBool (AOP (left), false);
  m6502_emitBranch ("beq", tlbl0);
  asmopToBool (AOP (right), false);
  m6502_emitBranch ("beq", tlbl0);
  m6502_loadRegFromConst (m6502_reg_a, 1);
  m6502_emitBranch ("bra", tlbl);
  m6502_safeEmitLabel (tlbl0);
  //  m6502_dirtyReg (m6502_reg_a);
  m6502_loadRegFromConst (m6502_reg_a, 0);
  m6502_safeEmitLabel (tlbl);
  m6502_dirtyReg (m6502_reg_a);

  //  m6502_useReg (m6502_reg_a);
  //  m6502_freeReg (m6502_reg_a);

  m6502_storeRegToFullAop (m6502_reg_a, AOP (result), false);
  pullOrFreeReg (m6502_reg_a, needpulla);

  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * genOrOp - for || operation
 *************************************************************************/
static void genOrOp (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  symbol *tlbl, *tlbl0;
  bool needpulla;

  m6502_emitComment (TRACEGEN, __func__);

  /* note here that || operations that are in an
     if statement are taken away by backPatchLabels
     only those used in arthmetic operations remain */
  m6502_aopOp (left, ic);
  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);
  m6502_printIC (ic);

  tlbl = m6502_safeNewiTempLabel (NULL);
  tlbl0 = m6502_safeNewiTempLabel (NULL);

  needpulla = pushRegIfSurv (m6502_reg_a);
  asmopToBool (AOP (left), false);
  m6502_emitBranch ("bne", tlbl0);
  asmopToBool (AOP (right), false);
  m6502_emitBranch ("bne", tlbl0);
  m6502_loadRegFromConst (m6502_reg_a, 0);
  m6502_emitBranch ("bra", tlbl);
  m6502_safeEmitLabel (tlbl0);
  //  m6502_dirtyReg (m6502_reg_a);
  m6502_loadRegFromConst (m6502_reg_a, 1);
  m6502_safeEmitLabel (tlbl);
  m6502_dirtyReg (m6502_reg_a);

  //  m6502_useReg (m6502_reg_a);
  //  m6502_freeReg (m6502_reg_a);

  m6502_storeRegToFullAop (m6502_reg_a, AOP (result), false);
  pullOrFreeReg (m6502_reg_a, needpulla);

  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * m6502_isLiteralBit - test if lit == 2^n
 *************************************************************************/
int
m6502_isLiteralBit (unsigned long long lit)
{
  int idx;

  for (idx = 0; idx < 64; idx++)
    if (lit == 1ull<<idx)
      return idx + 1;
  return 0;
}

/**************************************************************************
 * m6502_litmask - return the mask based on the operand size
 *************************************************************************/
unsigned long long
m6502_litmask (int size)
{
  unsigned long long ret = 0xffffffffffffffffull;
  if (size == 1)
    ret = 0xffull;
  else if (size == 2)
    ret= 0xffffull;
  else if (size == 4)
    ret = 0xffffffffull;
  return ret;
}

static const char * expand_symbols (iCode * ic, const char *inlin)
{
  const char *begin = NULL, *p = inlin;
  bool inIdent = false;
  struct dbuf_s dbuf;

  dbuf_init (&dbuf, 128);

  while (*p) {
    if (inIdent) {
      if ('_' == *p || isalnum (*p))
        /* in the middle of identifier */
        ++p;
      else {
        /* end of identifier */
        symbol *sym, *tempsym;
        char *symname = Safe_strndup (p + 1, p - begin - 1);

        inIdent = 0;

        tempsym = newSymbol (symname, ic->level);
        tempsym->block = ic->block;
        sym = (symbol *) findSymWithLevel (SymbolTab, tempsym);
        if (!sym) {
          dbuf_append (&dbuf, begin, p - begin);
        } else {
          asmop *aop = aopForSym (ic, sym);
          const char *l = aopAdrStr (aop, aop->size - 1, true);

          if ('#' == *l)
            l++;
          sym->isref = 1;
          if (sym->level && !sym->allocreq && !sym->ismyparm) {
            werror (E_ID_UNDEF, sym->name);
            werror (W_CONTINUE,
                    "  Add 'volatile' to the variable declaration so that it\n"
                    "  can be referenced within inline assembly");
          }
          dbuf_append_str (&dbuf, l);
        }
        Safe_free (symname);
        begin = p++;
      }
    } else if ('_' == *p) {
      /* begin of identifier */
      inIdent = true;
      if (begin)
        dbuf_append (&dbuf, begin, p - begin);
      begin = p++;
    } else {
      if (!begin)
        begin = p;
      p++;
    }
  }

  if (begin)
    dbuf_append (&dbuf, begin, p - begin);

  return dbuf_detach_c_str (&dbuf);
}

/**************************************************************************
 * genInline - write the inline code out
 *************************************************************************/
static void
m6502_genInline (iCode * ic)
{
  char *buf, *bp, *begin;
  const char *expanded;
  bool inComment = false;

  m6502_emitComment (TRACEGEN, __func__);

  genLine.lineElement.isInline += (!options.asmpeep);

  buf = bp = begin = Safe_strdup (IC_INLINE (ic));

  /* emit each line as a code */
  while (*bp) {
    switch (*bp) {
    case ';':
      inComment = true;
      ++bp;
      break;

    case '\x87':
    case '\n':
      inComment = false;
      *bp++ = '\0';
      expanded = expand_symbols (ic, begin);
      emitcode (expanded, NULL);
      dbuf_free (expanded);
      begin = bp;
      break;

    default:
      /* Add \n for labels, not dirs such as c:\mydir */
      if (!inComment && (*bp == ':') && (isspace ((unsigned char) bp[1]))) {
        ++bp;
        *bp = '\0';
        ++bp;
        emitcode (begin, NULL);
        begin = bp;
      }
      else
        ++bp;
      break;
    }
  }
  if (begin != bp) {
    expanded = expand_symbols (ic, begin);
    emitcode (expanded, NULL);
    dbuf_free (expanded);
  }

  Safe_free (buf);

  /* consumed; we can free it here */
  dbuf_free (IC_INLINE (ic));

  genLine.lineElement.isInline -= (!options.asmpeep);
}

/**************************************************************************
 * genGetByte - generates code to get a single byte
 *************************************************************************/
static void
genGetByte (const iCode *ic)
{
  operand *left   = IC_LEFT (ic);
  operand *right  = IC_RIGHT (ic);
  operand *result = IC_RESULT (ic);
  int offset;

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (left, ic);
  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);

  offset = (int) ulFromVal (right->aop->aopu.aop_lit) / 8;
  m6502_transferAopAop(left->aop, offset, result->aop, 0);

  m6502_freeAsmop (result, NULL);
  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (left, NULL);
}

/**************************************************************************
 * genGetWord - generates code to get a 16-bit word
 *************************************************************************/
static void
genGetWord (const iCode *ic)
{
  operand *left   = IC_LEFT (ic);
  operand *right  = IC_RIGHT (ic);
  operand *result = IC_RESULT (ic);
  int offset;

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (left, ic);
  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);

  offset = (int) ulFromVal (right->aop->aopu.aop_lit) / 8;
  m6502_transferAopAop(left->aop, offset, result->aop, 0);
  m6502_transferAopAop(left->aop, offset+1, result->aop, 1);

  m6502_freeAsmop (result, NULL);
  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (left, NULL);
}

/**************************************************************************
 * decodePointerOffset - decode a pointer offset operand into a
 *                    literal offset and a rematerializable offset
 *************************************************************************/
static void decodePointerOffset (operand * opOffset, int * litOffset, char ** rematOffset)
{
  *litOffset = 0;
  *rematOffset = NULL;

  if (!opOffset)
    return;

  if (IS_OP_LITERAL (opOffset))
    {
      *litOffset = (int)operandLitValue (opOffset);
    }
  else if (IS_ITEMP (opOffset) && OP_SYMBOL (opOffset)->remat)
    {
      asmop * aop = aopForRemat (OP_SYMBOL (opOffset));

      if (aop->type == AOP_LIT)
	*litOffset = (int) floatFromVal (aop->aopu.aop_lit);
      else if (aop->type == AOP_IMMD)
	*rematOffset = aop->aopu.aop_immd;
    }
  else
    wassertl (0, "Pointer get/set with non-constant offset");
}

/**************************************************************************
 * does a BIT A with a constant, even for non-65C02
 *************************************************************************/
// TODO: lookup table for each new const?
static void bitAConst(int val)
{
  wassertl (val >= 0 && val <= 0xff, "bitAConst()");
  if (IS_MOS65C02)
    {
      m6502_emitOp ("bit", IMMDFMT, (unsigned int)val);
    } 
  else 
    {
      reg_info *reg=m6502_getFreeByteReg();
      if(reg)
        {
          m6502_loadRegFromConst(reg,val);
          storeRegTempAlways (reg, true);
          m6502_emitRegTempOp ("bit", m6502_getLastTempOfs() );
	  m6502_loadRegTemp(NULL);
        }
      else
        {
          storeRegTemp (m6502_reg_a, true);
          m6502_emitOp ("and", IMMDFMT, (unsigned int)val);
	  m6502_loadRegTempNoFlags (m6502_reg_a, true);
        }
    }
}

/**************************************************************************
 * genUnpackBits - generates code for unpacking bits
 *************************************************************************/
static void genUnpackBits (operand * result, operand * left, operand * right, iCode * ifx)
{
  int offset = 0;               /* result byte offset */
  int rsize;                    /* result size */
  int rlen = 0;                 /* remaining bitfield length */
  unsigned blen;                /* bitfield length */
  unsigned bstr;                /* bitfield starting bit within byte */
  sym_link *etype;              /* bitfield type information */
  int litOffset = 0;
  char * rematOffset = NULL;
  bool needpulla = false;
  bool needpully = false;
  bool needpullx = false;
  m6502_emitComment (TRACEGEN, __func__);

  decodePointerOffset (right, &litOffset, &rematOffset);
  etype = getSpec (operandType (result));
  rsize = getSize (operandType (result));
  blen = SPEC_BLEN (etype);
  bstr = SPEC_BSTR (etype);

  //  needpulla = pushRegIfSurv (m6502_reg_a);
  needpulla = storeRegTempIfSurv (m6502_reg_a);

  // TODO: enable restoring from DPTR
  if (!IS_AOP_XY (AOP (left)))
    {
      needpullx = storeRegTempIfSurv (m6502_reg_x);
      needpully = storeRegTempIfSurv (m6502_reg_y);
    }

  int yoff= setupDPTR(left, litOffset, rematOffset, false);

  /* dptr now contains the address */

  if (ifx && blen <= 8)
    {
      m6502_loadRegFromConst(m6502_reg_y, yoff);
      m6502_emitOp ("lda", INDFMT_IY, "DPTR");
      if (blen < 8)
	m6502_emitOp ("and", IMMDFMT, (((unsigned char) - 1) >> (8 - blen)) << bstr);

      //      m6502_emitOp("php", "");//TODO
      m6502_loadOrFreeRegTemp (m6502_reg_y, needpully);
      m6502_loadOrFreeRegTemp (m6502_reg_x, needpullx);
      m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
      //      m6502_emitOp("plp", "");
      m6502_emitOp("ERROR", "%s: unimplemented ifx & blen<=8",__func__);
      m6502_genIfxJump (ifx, "z");
      return;
    }

  if(ifx)
    m6502_emitOp("ERROR", "%s: unimplemented ifx",__func__);

  /* If the bitfield length is less than a byte */
  if (blen < 8)
    {
      m6502_loadRegFromConst(m6502_reg_y, yoff);
      m6502_emitOp ("lda", INDFMT_IY, "DPTR");
      m6502_AccRsh (bstr, false);
      m6502_emitOp ("and", IMMDFMT, ((unsigned char) - 1) >> (8 - blen));
      m6502_useReg(m6502_reg_a);
      if (!SPEC_USIGN (etype) && !IS_BOOLEAN (etype))
	{
	  // signed bitfield
	  symbol *tlbl = m6502_safeNewiTempLabel (NULL);

	  bitAConst(1 << (blen - 1));
          m6502_emitBranch ("beq", tlbl);
	  m6502_emitOp ("ora", IMMDFMT, (unsigned char) (0xff << blen));
	  m6502_safeEmitLabel (tlbl);
	}
      m6502_storeRegToAop (m6502_reg_a, AOP (result), offset++);
      goto finish;
    }

  /* Bit length is greater than 7 bits. In this case, copy  */
  /* all except the partial byte at the end                 */
  for (rlen = blen; rlen >= 8; rlen -= 8)
    {
      m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
      m6502_emitOp ("lda", INDFMT_IY, "DPTR");
      if (rlen > 8 && AOP_TYPE (result) == AOP_REG)
	m6502_pushReg (m6502_reg_a, true);
      else
	m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);
      offset++;
    }

  /* Handle the partial byte at the end */
  if (rlen)
    {
      m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
      m6502_emitOp ("lda", INDFMT_IY, "DPTR");
      m6502_emitOp ("and", IMMDFMT, ((unsigned char) - 1) >> (8 - rlen));
      m6502_useReg(m6502_reg_a);
      if (!SPEC_USIGN (etype) && !IS_BOOLEAN (etype))
	{
	  /* signed bitfield */
	  symbol *tlbl = m6502_safeNewiTempLabel (NULL);
	  // FIXME: works but very ugly
	  bitAConst(1 << (rlen - 1));
          m6502_emitBranch ("beq", tlbl);
	  m6502_emitOp ("ora", IMMDFMT, (unsigned char) (0xff << rlen));
	  m6502_safeEmitLabel (tlbl);
	}
      m6502_storeRegToAop (m6502_reg_a, AOP (result), offset++);
    }
  if (blen > 8 && AOP_TYPE (result) == AOP_REG)
    {
      m6502_pullReg (AOP (result)->aopu.aop_reg[0]);
    }

 finish:
  if (offset < rsize)
    {
      rsize -= offset;
      if (SPEC_USIGN (etype) || IS_BOOLEAN (etype))
	{
	  while (rsize--)
	    m6502_storeConstToAop (0, AOP (result), offset++);
	}
      else
	{
	  /* signed bitfield: sign extension with 0x00 or 0xff */
	  m6502_signExtendReg(m6502_reg_a);
	  while (rsize--)
	    m6502_storeRegToAop (m6502_reg_a, AOP (result), offset++);
	}
    }

  m6502_loadOrFreeRegTemp (m6502_reg_y, needpully);
  m6502_loadOrFreeRegTemp (m6502_reg_x, needpullx);
  m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
}

/**************************************************************************
 * genUnpackBitsImmed - generates code for unpacking bits
 *************************************************************************/
static void genUnpackBitsImmed (operand * left, operand *right, operand * result, iCode * ic, iCode * ifx)
{
  int size;
  int offset = 0;               /* result byte offset */
  int litOffset = 0;
  char * rematOffset = NULL;
  int rsize;                    /* result size */
  int rlen = 0;                 /* remaining bitfield length */
  sym_link *etype;              /* bitfield type information */
  unsigned blen;                /* bitfield length */
  unsigned bstr;                /* bitfield starting bit within byte */
  asmop *derefaop;
  bool delayed_a = false;
  bool assigned_a = false;
  bool needpulla = false;

  m6502_emitComment (TRACEGEN, __func__);

  decodePointerOffset (right, &litOffset, &rematOffset);
  wassert (rematOffset==NULL);

  m6502_aopOp (result, ic);
  size = AOP_SIZE (result);

  derefaop = aopDerefAop (AOP (left), litOffset);
  m6502_freeAsmop (left, NULL);
  derefaop->size = size;

  etype = getSpec (operandType (result));
  rsize = getSize (operandType (result));
  blen = SPEC_BLEN (etype);
  bstr = SPEC_BSTR (etype);

  needpulla = pushRegIfSurv (m6502_reg_a);

  /* if the bitfield is a single bit in the direct page */
  if (blen == 1 && derefaop->type == AOP_DIR)
    {
      if (!ifx && bstr)
	{
	  symbol *tlbl = m6502_safeNewiTempLabel (NULL);

	  // FIXME: unimplemented
	  m6502_loadRegFromConst (m6502_reg_a, 0);
	  m6502_unimplemented("genUnpackBitsImmed");
 	  //          m6502_emitOp ("brclr", "#%d,%s,%05d$", bstr, aopAdrStr (derefaop, 0, false), m6502_safeLabelNum ((tlbl)));
	  if (SPEC_USIGN (etype))
	    m6502_rmwWithReg ("inc", m6502_reg_a);
	  else
	    m6502_rmwWithReg ("dec", m6502_reg_a);

	  m6502_safeEmitLabel (tlbl);
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);
	  if (AOP_TYPE (result) == AOP_REG && AOP(result)->aopu.aop_reg[offset]->rIdx == A_IDX)
	    assigned_a = true;

	  m6502_freeReg (m6502_reg_a);
	  offset++;
	  goto finish;
	}
      else if (ifx)
	{
	  symbol *tlbl = m6502_safeNewiTempLabel (NULL);
	  symbol *jlbl;
	  char *inst;

	  // FIXME
	  if (IC_TRUE (ifx))
	    {
	      jlbl = IC_TRUE (ifx);
	      inst = "brclr";
	    }
	  else
	    {
	      jlbl = IC_FALSE (ifx);
	      inst = "brset";
	    }
	  m6502_emitOp (inst, "#%d,%s,%05d$", bstr, aopAdrStr (derefaop, 0, false), m6502_safeLabelNum ((tlbl)));
	  m6502_emitBranch ("jmp", jlbl);
	  m6502_safeEmitLabel (tlbl);
	  ifx->generated = 1;
	  offset++;
	  goto finish;
	}
    }

  /* If the bitfield length is less than a byte */
  if (blen < 8)
    {
      m6502_loadRegFromAop (m6502_reg_a, derefaop, 0);
      if (!ifx)
	{
	  // TODO: inefficient if just getting flags
	  m6502_AccRsh (bstr, false);
	  m6502_emitOp ("and", IMMDFMT, ((unsigned char) - 1) >> (8 - blen));
	  if (!SPEC_USIGN (etype))
	    {
	      /* signed bitfield */
	      symbol *tlbl = m6502_safeNewiTempLabel (NULL);
	      bitAConst(1 << (blen - 1));
              m6502_emitBranch ("beq", tlbl);
	      m6502_emitOp ("ora", IMMDFMT, (unsigned char) (0xff << blen));
	      m6502_safeEmitLabel (tlbl);
	    }
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);
	  if (AOP_TYPE (result) == AOP_REG && AOP(result)->aopu.aop_reg[offset]->rIdx == A_IDX)
	    assigned_a = true;
	}
      else
	{
	  m6502_emitOp ("and", IMMDFMT, (((unsigned char) - 1) >> (8 - blen)) << bstr);
	}
      offset++;
      goto finish;
    }

  /* Bit field did not fit in a byte. Copy all
     but the partial byte at the end.  */
  for (rlen = blen; rlen >= 8; rlen -= 8)
    {
      if (assigned_a && !delayed_a)
	{
	  m6502_pushReg (m6502_reg_a, true);
	  delayed_a = true;
	}
      m6502_loadRegFromAop (m6502_reg_a, derefaop, offset);
      if (!ifx)
	{
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);
	  if (AOP_TYPE (result) == AOP_REG && AOP(result)->aopu.aop_reg[offset]->rIdx == A_IDX)
	    assigned_a = true;
	}
      else
        {
	  m6502_emitCmp(m6502_reg_a, 0);
        }
      offset++;
    }

  /* Handle the partial byte at the end */
  if (rlen)
    {
      if (assigned_a && !delayed_a)
	{
	  m6502_pushReg (m6502_reg_a, true);
	  delayed_a = true;
	}
      m6502_loadRegFromAop (m6502_reg_a, derefaop, offset);
      m6502_emitOp ("and", IMMDFMT, ((unsigned char) - 1) >> (8 - rlen));
      if (!SPEC_USIGN (etype))
	{
	  /* signed bitfield */
	  symbol *tlbl = m6502_safeNewiTempLabel (NULL);

	  bitAConst (1 << (rlen - 1));
          m6502_emitBranch ("beq", tlbl);
	  m6502_emitOp ("ora", IMMDFMT, (unsigned char) (0xff << rlen));
	  m6502_safeEmitLabel (tlbl);
	}
      m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);
      if (AOP_TYPE (result) == AOP_REG && AOP(result)->aopu.aop_reg[offset]->rIdx == A_IDX)
	assigned_a = true;
      offset++;
    }

 finish:
  if (offset < rsize)
    {
      rsize -= offset;
      if (SPEC_USIGN (etype))
	{
	  while (rsize--)
	    m6502_storeConstToAop (0, AOP (result), offset++);
	}
      else
	{
	  if (assigned_a && !delayed_a)
	    {
	      m6502_pushReg (m6502_reg_a, true);
	      delayed_a = true;
	    }

	  /* signed bitfield: sign extension with 0x00 or 0xff */
	  m6502_signExtendReg(m6502_reg_a);
	  while (rsize--)
	    m6502_storeRegToAop (m6502_reg_a, AOP (result), offset++);
	}
    }

  m6502_freeAsmop (NULL, derefaop);
  m6502_freeAsmop (result, NULL);

  if (ifx && !ifx->generated)
    m6502_genIfxJump (ifx, "z");

  if (delayed_a)
    m6502_pullReg (m6502_reg_a);

  // TODO? wrong plac?
  pullOrFreeReg (m6502_reg_a, needpulla);
}

/**************************************************************************
 * genDataPointerGet - generates code when ptr offset is known
 *************************************************************************/
static void genDataPointerGet (operand * left, operand * right, operand * result, iCode * ic, iCode * ifx)
{
  int size;
  int litOffset = 0;
  char * rematOffset = NULL;
  asmop *derefaop;
  bool needpulla = false;

  m6502_emitComment (TRACEGEN, __func__);

  decodePointerOffset (right, &litOffset, &rematOffset);
  wassert (rematOffset==NULL);

  m6502_aopOp (result, ic);
  size = AOP_SIZE (result);

  // TODO: aopDerefAop(IMMD(_ftest_a_65536_8)), why?
  derefaop = aopDerefAop (AOP (left), litOffset);
  m6502_freeAsmop (left, NULL);
  derefaop->size = size;

  if (ifx)
    needpulla = storeRegTempIfSurv (m6502_reg_a);

  if (IS_AOP_XY (AOP (result)))
    m6502_loadRegFromAop (m6502_reg_xy, derefaop, 0);
  else while (size--)
	 {
	   if (!ifx)
	     m6502_transferAopAop (derefaop, size, AOP (result), size);
	   else
	     m6502_loadRegFromAop (m6502_reg_a, derefaop, size);
	 }

  m6502_freeAsmop (NULL, derefaop);
  m6502_freeAsmop (result, NULL);

  if (ifx && !ifx->generated)
    {
      m6502_loadRegTempNoFlags (m6502_reg_a, needpulla);
      m6502_genIfxJump (ifx, "z");
    }
  else
    {
      if (needpulla)
        m6502_loadRegTemp (NULL);
    }
}

/**************************************************************************
 * genPointerGet - generate code for pointer get
 *************************************************************************/
static void genPointerGet (iCode * ic, iCode * ifx)
{
  operand *right  = IC_RIGHT (ic);
  operand *left = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);
  int size, offset;
  int litOffset = 0;
  char * rematOffset = NULL;
  bool bit_field = IS_BITVAR (operandType (result)) && (SPEC_BLEN (operandType (result))%8);
  bool needpulla = false;

  m6502_emitComment (TRACEGEN, __func__);
  // result = *(left (register offset) + right (remat+literal_offset) )

  size = getSize (operandType (result));
  if (size > 1)
    ifx = NULL;

  m6502_aopOp (left, ic);
  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);

  /* if left is rematerialisable */
  if (AOP_TYPE (left) == AOP_IMMD || AOP_TYPE (left) == AOP_LIT)
    {
      /* if result is bit variable type */
      if (bit_field)
	genUnpackBitsImmed (left, right, result, ic, ifx);
      else
	genDataPointerGet (left, right, result, ic, ifx);

      goto release;
    }

  /* if bit then unpack */
  if (bit_field)
    {
      genUnpackBits (result, left, right, ifx);
      goto release;
    }

  decodePointerOffset (right, &litOffset, &rematOffset);

  m6502_printIC (ic);

  /* force offset to signed 16-bit range */
  litOffset &= 0xffff;
  if (litOffset & 0x8000)
    litOffset = 0x10000 - litOffset;

  m6502_emitComment (TRACEGEN|VVDBG, "  %s src: %s size=%d loff=%d rmoff=%s",
		     __func__, aopName(AOP(left)), size, litOffset, rematOffset );
  m6502_emitComment (TRACEGEN|VVDBG, "  %s dst: %s size=%d",
		     __func__, aopName(AOP(result)), AOP_SIZE(result) );

  if ( m6502_findRegAop(AOP(result), 0) == m6502_reg_a )
    m6502_dirtyReg(m6502_reg_a);

  if ( m6502_reg_y->aop && m6502_sameRegs(m6502_reg_y->aop, AOP(result)) )
    m6502_dirtyReg(m6502_reg_y);

  if (AOP_TYPE (left) == AOP_REG)
    {
      char hstring[3] ="";
      char lstring[3] ="??";

      if (AOP(left)->aopu.aop_reg[0]->isLitConst)
	sprintf(lstring,"%02x",AOP(left)->aopu.aop_reg[0]->litConst);

      if (AOP_SIZE(left) == 2)
	{
	  if (AOP(left)->aopu.aop_reg[1]->isLitConst)
	    sprintf(hstring,"%02x",AOP(left)->aopu.aop_reg[1]->litConst);
	  else
	    sprintf(hstring,"??");
	}
      m6502_emitComment (TRACEGEN|VVDBG, "    %s (%s) = 0x%s%s",
			 __func__, aopName(AOP(left)), hstring, lstring );

    }

  if (AOP_TYPE (left) == AOP_DIR 
      && !rematOffset && litOffset >= 0 && litOffset <= 256-size)
    {
      // pointer is already in zero page & 8-bit offset
      m6502_emitComment (TRACEGEN|VVDBG, "      %s - pointer already in zp", __func__);
      bool needloady = storeRegTempIfSurv(m6502_reg_y);

#if 0
      // seem to make perf worse
      if (size == 1 && litOffset == 0
          && ( /*m6502_reg_x->isDead || */ (m6502_reg_x->isLitConst && m6502_reg_x->litConst == 0) ) )
	{
	  // [aa,x] x == 0
	  m6502_loadRegFromConst(m6502_reg_x,0);
	  m6502_emitOp ("lda", "[%s,x]", aopAdrStr ( AOP(left), 0, true ) );
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
	  goto release;
	}
#endif

      if ( m6502_sameRegs(AOP(left), AOP(result)) )
	{
	  // pointer and destination is the same - need avoid overwriting
	  m6502_emitComment (TRACEGEN|VVDBG, "    %s - sameregs off:%d",
			     __func__, litOffset);

          if (m6502_reg_a->aop && m6502_reg_a->aop->type==AOP_DIR
	      && m6502_sameRegs(m6502_reg_a->aop, AOP(result)) )
            {
	      m6502_emitComment (TRACEGEN|VVDBG, "    %s - dirty A", __func__);
	      m6502_dirtyReg (m6502_reg_a);
            }
	  needpulla = storeRegTempIfSurv (m6502_reg_a);

	  for (int i=size-1; i>=0; i--)
	    {
	      m6502_loadRegFromConst(m6502_reg_y, litOffset + i);
	      m6502_emitOp ("lda", INDFMT_IY, AOP(left)->aopu.aop_dir);
	      if (i>1)
		{
		  m6502_storeRegToAop (m6502_reg_a, AOP (result), i);
		}
	      else if (i==1)
		{
 		  m6502_fastSaveAi(m6502_reg_x);
		}
	      else if (i==0)
		{
		  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
		  if (size>1)
		    {
		      m6502_fastRestoreA();
		      m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
		    }
		}
	    }
	}
      else
        {
	  // otherwise use [aa],y
          if (!IS_AOP_WITH_A(AOP(result)))
            needpulla = storeRegTempIfSurv (m6502_reg_a);

	  if (IS_AOP_XA(AOP(result)) || IS_AOP_XY(AOP(result)))
	    {
	      // reverse order so A is last
	      m6502_emitComment (TRACEGEN|VVDBG, "    %s: dest XA/XY", __func__);
	      for (int i=size-1; i>=0; i--)
		{
		  m6502_loadRegFromConst(m6502_reg_y, litOffset + i);
		  m6502_emitOp ("lda", INDFMT_IY, aopAdrStr ( AOP(left), 0, true ) );
		  m6502_storeRegToAop (m6502_reg_a, AOP (result), i);
		}
	    }
	  else
	    {
	      // forward order
	      m6502_emitComment (TRACEGEN|VVDBG, "    %s: dest generic", __func__);
	      for (int i=0; i<size; i++)
		{
		  m6502_loadRegFromConst(m6502_reg_y, litOffset + i);
		  m6502_emitOp ("lda", INDFMT_IY, aopAdrStr ( AOP(left), 0, true ) );
		  m6502_storeRegToAop (m6502_reg_a, AOP (result), i);
		}
	    }
	}
      m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
      m6502_loadOrFreeRegTemp (m6502_reg_y, needloady);
      goto release;
    }

  // try absolute indexed
#if 0
  // allow index to be in memory
  if (rematOffset
      && ( AOP_SIZE(left)==1
           || ( AOP_TYPE(left) == AOP_REG && AOP(left)->aopu.aop_reg[1]->isLitConst ) ) )
#else
    // index can only be a register
    if (AOP_TYPE(left) == AOP_REG &&
        (AOP_SIZE(left) == 1|| AOP(left)->aopu.aop_reg[1]->isLitConst ))
#endif
      {
        m6502_emitComment (TRACEGEN|VVDBG,"    %s - absolute with 8-bit index", __func__);
        unsigned int hi_offset=0;
        char *dst_reg;
        char idx_reg;

        if(!rematOffset)
          rematOffset="0x0000";

        if(AOP_SIZE(left)==2)
          {
	    hi_offset=(AOP(left)->aopu.aop_reg[1]->litConst)<<8;
            m6502_freeReg(m6502_reg_x);
          }

        if(AOP_TYPE(left)==AOP_REG)
	  {
	    switch(AOP(left)->aopu.aop_reg[0]->rIdx)
	      {
	      case X_IDX: idx_reg='x'; break;
	      case Y_IDX: idx_reg='y'; break;
	      case A_IDX: idx_reg='A'; break;
	      default: idx_reg='E'; break;
	      }
	  }
	else
	  {
	    idx_reg='M';
	  }

        if(IS_AOP_XA(AOP(result)))
          {
            if(idx_reg=='A')
              m6502_transferRegReg(m6502_reg_a, m6502_reg_y, true);
            idx_reg='y';
            dst_reg="lda";
          }
        else if(AOP_TYPE(result)==AOP_REG)
	  {
	    switch(AOP(result)->aopu.aop_reg[0]->rIdx)
	      {
	      case A_IDX: dst_reg="lda"; break;
	      case X_IDX: dst_reg="ldx"; break;
	      case Y_IDX: dst_reg="ldy"; break;
	      default: dst_reg="ERROR"; break;
	      }
	  }
	else
	  {
	    dst_reg="MEM";
	  }

        bool px = false;
        bool py = false;
        bool pa = false;

        if(idx_reg=='A' || idx_reg=='M')
	  {
            if(idx_reg=='A' && m6502_reg_a->aop==&m6502_tsxaop && AOP_TYPE(result)==AOP_SOF)
              {
                // A is a stack offset
                idx_reg='x';
                px = storeRegTempIfSurv(m6502_reg_x);
                m6502_emitTSX();
                hi_offset+=(m6502_reg_a->stackOffset-m6502_reg_x->stackOffset);
                m6502_emitComment (TRACEGEN|VVDBG, "    %s: sofx:%d  sofa:%d  sof:%d",
				   __func__, m6502_reg_x->stackOffset, m6502_reg_a->stackOffset,
				   m6502_reg_a->stackOffset-m6502_reg_x->stackOffset);
              }
            else if(dst_reg[2]=='x' || m6502_reg_x->aop==&m6502_tsxaop || AOP_TYPE(result)==AOP_SOF)
	      {
		py = storeRegTempIfSurv(m6502_reg_y);
		m6502_loadRegFromAop(m6502_reg_y, AOP(left), 0 );
		idx_reg='y';
                if(AOP_TYPE(result)==AOP_SOF)
                  px = storeRegTempIfSurv(m6502_reg_x);
	      }
	    else
	      {
		// FIXME: should check for a free reg to avoid saving if possible
		px = storeRegTempIfSurv(m6502_reg_x);
		m6502_loadRegFromAop(m6502_reg_x, AOP(left), 0 );
		idx_reg='x';
	      }
	  }

        if(dst_reg[2] == idx_reg || dst_reg[0]=='M')
	  {
	    //             m6502_loadRegFromAop (m6502_reg_a, AOP (right), 0);
	    //             dst_reg="lda";
	    pa = storeRegTempIfSurv(m6502_reg_a);
            for(offset=0; offset<AOP_SIZE(result); offset++)
              {
		m6502_emitOp("lda", "(%s+0x%04x+%d),%c",
			     rematOffset, litOffset+hi_offset, offset, idx_reg );
		m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);
              }
	    m6502_loadOrFreeRegTemp(m6502_reg_a,pa);
	  }
	else
	  {
 	    m6502_emitOp(dst_reg, "(%s+0x%04x+%d),%c",
			 rematOffset, litOffset+hi_offset, 0, idx_reg );
            if(IS_AOP_XA(AOP(result)))
              {
		m6502_emitOp("ldx", "(%s+0x%04x+%d),%c",
			     rematOffset, litOffset+hi_offset, 1, idx_reg );
                 
              }              
            if(IS_AOP_XY(AOP(result)))
              {
		m6502_emitOp("lda", "(%s+0x%04x+%d),%c",
			     rematOffset, litOffset+hi_offset, 1, idx_reg );
                m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);  
              }
	  }

        m6502_loadOrFreeRegTemp(m6502_reg_x,px);
        m6502_loadOrFreeRegTemp(m6502_reg_y,py);
        goto release;
      }

  bool restore_a_from_dptr = false;
  bool restore_x_from_dptr = false;
  bool needloadx = false;
  bool need_x = false;

  need_x= (AOP_TYPE(left)==AOP_SOF || AOP_TYPE(result)==AOP_SOF);
  bool use_dptr = true;
  asmop *ptr_aop = m6502_reg_a->aop;
  int yoff;

  if(IS_AOP_XA(AOP(left)) && !rematOffset)
    {
      if(ptr_aop && ptr_aop->type==AOP_DIR && !m6502_sameRegs(ptr_aop, AOP(result)))
        {
	  use_dptr=false;
        }
      else
        {
          restore_a_from_dptr = !m6502_reg_a->isDead;
          if(need_x)
            restore_x_from_dptr = !m6502_reg_x->isDead;
        }
    }
  else
    {
      needpulla = storeRegTempIfSurv (m6502_reg_a);
      if(need_x)
	needloadx = storeRegTempIfSurv(m6502_reg_x);
    }
  bool needloady = storeRegTempIfSurv(m6502_reg_y);

  if(use_dptr)
    yoff = setupDPTR(left, litOffset, rematOffset, false);
  else
    yoff=litOffset;

  m6502_emitComment (TRACEGEN|VVDBG, "        %s: generic path", __func__);

  if (IS_AOP_XA (AOP (result)) || IS_AOP_XY (AOP (result)))
    {
      m6502_loadRegFromConst(m6502_reg_y, yoff + 1);
      if(use_dptr)
        m6502_emitOp ("lda", INDFMT_IY, "DPTR");
      else
        m6502_emitOp("lda", INDFMT_IY, ptr_aop->aopu.aop_dir);

      m6502_transferRegReg(m6502_reg_a, m6502_reg_x, true);
      m6502_loadRegFromConst(m6502_reg_y, yoff + 0);
      if(use_dptr)
        m6502_emitOp ("lda", INDFMT_IY, "DPTR");
      else
        m6502_emitOp("lda", INDFMT_IY, ptr_aop->aopu.aop_dir);

      if(IS_AOP_XY (AOP (result)))
        m6502_transferRegReg(m6502_reg_a, m6502_reg_y, true);
    }
  else
    {
      for (offset=0; offset<size; offset++)
        {
          m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
	  if(use_dptr)
	    m6502_emitOp ("lda", INDFMT_IY, "DPTR");
	  else
	    m6502_emitOp("lda", INDFMT_IY, ptr_aop->aopu.aop_dir);

          m6502_storeRegToAop (m6502_reg_a, AOP (result), offset);
        }
    }
  m6502_loadOrFreeRegTemp (m6502_reg_y, needloady);
  m6502_loadOrFreeRegTemp (m6502_reg_x, needloadx);
  m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);

  if(restore_x_from_dptr)
    loadRegFromDPTR(m6502_reg_x, 1);
  if(restore_a_from_dptr)
    loadRegFromDPTR(m6502_reg_a, 0);

 release:
  m6502_freeAsmop (left, NULL);
  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (result, NULL);

  if (ifx && !ifx->generated)
    m6502_genIfxJump (ifx, "z");
}

/**************************************************************************
 * genPackBits - generates code for packed bit storage
 *************************************************************************/
static void genPackBits (operand * result, operand * left, sym_link * etype, operand * right)
{
  int offset = 0;               /* source byte offset */
  int rlen = 0;                 /* remaining bitfield length */
  unsigned blen;                /* bitfield length */
  unsigned bstr;                /* bitfield starting bit within byte */
  unsigned long long  litval;   /* source literal value (if AOP_LIT) */
  unsigned char mask;           /* bitmask within current byte */
  int litOffset = 0;
  char *rematOffset = NULL;
  bool needpulla;

  m6502_emitComment (TRACEGEN, __func__);

  decodePointerOffset (left, &litOffset, &rematOffset);
  blen = SPEC_BLEN (etype);
  bstr = SPEC_BSTR (etype);

  m6502_emitComment (TRACEGEN, "%s - blen:%d bstr:%d", __func__, blen, bstr);

  needpulla = storeRegTempIfSurv (m6502_reg_a);

  if (AOP_TYPE (right) == AOP_REG)
    {
      /* Not optimal, but works for any register sources. */
      /* Just push the source values onto the stack and   */
      /* pull them off any needed. Better optimzed would  */
      /* be to do some of the shifting/masking now and    */
      /* push the intermediate result. */
      if (blen > 8)
        m6502_pushReg (AOP (right)->aopu.aop_reg[1], true);

      m6502_pushReg (AOP (right)->aopu.aop_reg[0], true);
    }

  int yoff= setupDPTR(result, litOffset, rematOffset, false);

  /* If the bitfield length is less than a byte */
  if (blen < 8)
    {
      mask = ((unsigned char) (0xFF << (blen + bstr)) | (unsigned char) (0xFF >> (8 - bstr)));

      if (AOP_TYPE (right) == AOP_LIT)
	{
	  // Case with a bitfield length <8 and literal source
	  litval = ullFromVal (AOP (right)->aopu.aop_lit);
	  litval <<= bstr;
	  litval &= (~mask) & 0xff;

	  m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
	  m6502_emitOp ("lda", INDFMT_IY, "DPTR");
	  if ((mask | litval) != 0xff)
	    m6502_emitOp ("and", IMMDFMT, (unsigned int)mask);

	  if (litval)
	    m6502_emitOp ("ora", IMMDFMT, (unsigned int)litval);

	  m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
	  m6502_emitOp ("sta", INDFMT_IY, "DPTR");
	  m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
	  return;
	}

      // Case with a bitfield length < 8 and arbitrary source
      if (AOP_TYPE (right) == AOP_REG)
	m6502_pullReg (m6502_reg_a);
      else
	m6502_loadRegFromAop (m6502_reg_a, AOP (right), 0);

      // shift and mask source value
      m6502_AccLsh (bstr);
      m6502_emitOp ("and", IMMDFMT, (unsigned int)(~mask) & 0xffu);
      storeRegTemp (m6502_reg_a, true);

      m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
      m6502_emitOp("lda", INDFMT_IY, "DPTR");
      m6502_emitOp("and", IMMDFMT, (unsigned int)mask);
      m6502_emitRegTempOp ("ora", m6502_getLastTempOfs() );
      m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
      m6502_emitOp ("sta", INDFMT_IY, "DPTR");
      //      m6502_loadRegTemp (m6502_reg_a);
      m6502_loadRegTemp (NULL);
      // TODO? redundant?
      m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
      return;
    }

  /* Bit length is greater than 7 bits. In this case, copy  */
  /* all except the partial byte at the end                 */
  for (rlen = blen; rlen >= 8; rlen -= 8)
    {
      if (AOP_TYPE (right) == AOP_REG)
        m6502_pullReg (m6502_reg_a);
      else
        m6502_loadRegFromAop (m6502_reg_a, AOP (right), offset);

      //          storeRegIndexed (m6502_reg_a, litOffset+offset, rematOffset);
      m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
      m6502_emitOp ("sta", INDFMT_IY, "DPTR");
      offset++;
    }

  /* If there was a partial byte at the end */
  if (rlen)
    {
      mask = (((unsigned char) - 1 << rlen) & 0xff);

      if (AOP_TYPE (right) == AOP_LIT)
	{
	  // Case with partial byte and literal source
	  litval = (int) ulFromVal (AOP (right)->aopu.aop_lit);
	  litval >>= (blen - rlen);
	  litval &= (~mask) & 0xff;
	  //          loadRegIndexed (m6502_reg_a, litOffset+offset, rematOffset);
	  m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
	  m6502_emitOp ("lda", INDFMT_IY, "DPTR");
	  if ((mask | litval) != 0xff)
	    m6502_emitOp ("and", IMMDFMT, (unsigned int)mask);

	  if (litval)
	    m6502_emitOp ("ora", IMMDFMT, (unsigned int)litval);

	  m6502_dirtyReg (m6502_reg_a);
	  //          storeRegIndexed (m6502_reg_a, litOffset+offset, rematOffset);
	  m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
	  m6502_emitOp ("sta", INDFMT_IY, "DPTR");
	  m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
	  return;
	}

      // Case with partial byte and arbitrary source
      if (AOP_TYPE (right) == AOP_REG)
	m6502_pullReg (m6502_reg_a);
      else
	m6502_loadRegFromAop (m6502_reg_a, AOP (right), offset);

      m6502_emitOp ("and", IMMDFMT, (unsigned int)(~mask) & 0xffu);
      storeRegTemp(m6502_reg_a, true);
      m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
      m6502_emitOp ("lda", INDFMT_IY, "DPTR");
      m6502_emitOp ("and", IMMDFMT, (unsigned int)mask);
      m6502_emitRegTempOp ("ora", m6502_getLastTempOfs() );
      m6502_loadRegTemp(NULL);
      m6502_emitOp ("sta", INDFMT_IY, "DPTR");
    }

  m6502_loadOrFreeRegTemp (m6502_reg_a, needpulla);
}

/**************************************************************************
 * genPackBitsImmed - generates code for packed bit storage
 *************************************************************************/
static void genPackBitsImmed (operand * result, operand * left, sym_link * etype, operand * right, iCode * ic)
{
  asmop *derefaop;
  int size;
  int offset = 0;               /* source byte offset */
  int rlen = 0;                 /* remaining bitfield length */
  unsigned blen;                /* bitfield length */
  unsigned bstr;                /* bitfield starting bit within byte */
  unsigned long long int litval;/* source literal value (if AOP_LIT) */
  unsigned char mask;           /* bitmask within current byte */
  bool needpulla;
  int litOffset = 0;
  char *rematOffset = NULL;
  
  m6502_emitComment (TRACEGEN, __func__);
  
  blen = SPEC_BLEN (etype);
  bstr = SPEC_BSTR (etype);
  
  m6502_aopOp (right, ic);
  size = AOP_SIZE (right);
  decodePointerOffset (left, &litOffset, &rematOffset);
  wassert (!rematOffset);
  
  derefaop = aopDerefAop (AOP (result), litOffset);
  m6502_freeAsmop (result, NULL);
  derefaop->size = size;
  
  /* if the bitfield is a single bit in the direct page */
  if (blen == 1 && derefaop->type == AOP_DIR)
    {
      if (AOP_TYPE (right) == AOP_LIT)
	{
	  litval = ullFromVal (AOP (right)->aopu.aop_lit);
	  // FIXME: unimplemented
	  m6502_unimplemented("genPackBitsImmed 1");
	  //m6502_emitOp ((litval & 1) ? "bset" : "bclr", "#%d,%s", bstr, aopAdrStr (derefaop, 0, false));
	}
      else
	{
	  symbol *tlbl1 = m6502_safeNewiTempLabel (NULL);
	  symbol *tlbl2 = m6502_safeNewiTempLabel (NULL);
      
	  // FIXME: unimplemented
	  m6502_unimplemented("genPackBitsImmed 2");
	  needpulla = pushRegIfSurv (m6502_reg_a);
	  m6502_loadRegFromAop (m6502_reg_a, AOP (right), 0);
	  m6502_emitOp ("lsr", "a");
	  m6502_emitBranch ("bcs", tlbl1);
	  m6502_emitOp ("bclr", "#%d,%s", bstr, aopAdrStr (derefaop, 0, false));
	  m6502_emitBranch ("bra", tlbl2);
	  m6502_safeEmitLabel (tlbl1);
	  m6502_emitOp ("bset", "#%d,%s", bstr, aopAdrStr (derefaop, 0, false));
	  m6502_safeEmitLabel (tlbl2);
	  pullOrFreeReg (m6502_reg_a, needpulla);
	}
      goto release;
    }
  
  /* If the bitfield length is less than a byte */
  if (blen < 8)
    {
      mask = ((unsigned char) (0xFF << (blen + bstr)) | (unsigned char) (0xFF >> (8 - bstr)));
    
      if (AOP_TYPE (right) == AOP_LIT)
        {
          // Case with a bitfield length <8 and literal source
          litval = (int) ulFromVal (AOP (right)->aopu.aop_lit);
          litval <<= bstr;
          litval &= (~mask) & 0xff;

          needpulla = pushRegIfSurv (m6502_reg_a);
          m6502_loadRegFromAop (m6502_reg_a, derefaop, 0);
          if ((mask | litval) != 0xff)
            m6502_emitOp ("and", IMMDFMT, (unsigned int)mask);

          if (litval)
            m6502_emitOp ("ora", IMMDFMT, (unsigned int)litval);

          m6502_dirtyReg (m6502_reg_a);
          m6502_storeRegToAop (m6502_reg_a, derefaop, 0);
          pullOrFreeReg (m6502_reg_a, needpulla);
          goto release;
        }

      // Case with a bitfield length < 8 and arbitrary source
      needpulla = pushRegIfSurv (m6502_reg_a);
      m6502_loadRegFromAop (m6502_reg_a, AOP (right), 0);
      /* shift and mask source value */
      m6502_AccLsh (bstr);
      m6502_emitOp ("and", IMMDFMT, (unsigned int)(~mask) & 0xffu);
      storeRegTemp(m6502_reg_a, true);
    
      m6502_loadRegFromAop (m6502_reg_a, derefaop, 0);
      m6502_emitOp("and", IMMDFMT, (unsigned int)mask);
      m6502_emitRegTempOp ("ora", m6502_getLastTempOfs() );
      m6502_storeRegToAop (m6502_reg_a, derefaop, 0);
    
      pullOrFreeReg (m6502_reg_a, needpulla);
      m6502_loadRegTemp(NULL);
      goto release;
    }
  
  /* Bit length is greater than 7 bits. In this case, copy  */
  /* all except the partial byte at the end                 */
  for (rlen = blen; rlen >= 8; rlen -= 8)
    {
      m6502_transferAopAop (AOP (right), offset, derefaop, offset);
      offset++;
    }
  
  /* If there was a partial byte at the end */
  if (rlen)
    {
      mask = (((unsigned char) - 1 << rlen) & 0xff);
    
      if (AOP_TYPE (right) == AOP_LIT)
	{
	  // Case with partial byte and literal source
	  litval = (int) ulFromVal (AOP (right)->aopu.aop_lit);
	  litval >>= (blen - rlen);
	  litval &= (~mask) & 0xff;
	  needpulla = pushRegIfSurv (m6502_reg_a);
	  m6502_loadRegFromAop (m6502_reg_a, derefaop, offset);
	  if ((mask | litval) != 0xff)
	    m6502_emitOp ("and", IMMDFMT, (unsigned int)mask);

	  if (litval)
	    m6502_emitOp ("ora", IMMDFMT, (unsigned int)litval);

	  m6502_dirtyReg (m6502_reg_a);
	  m6502_storeRegToAop (m6502_reg_a, derefaop, offset);
	  m6502_dirtyReg (m6502_reg_a);
	  pullOrFreeReg (m6502_reg_a, needpulla);
	  goto release;
	}
    
      // Case with partial byte and arbitrary source
      needpulla = pushRegIfSurv (m6502_reg_a);
      m6502_loadRegFromAop (m6502_reg_a, AOP (right), offset);
      m6502_emitOp ("and", IMMDFMT, (unsigned int)(~mask) & 0xffu);
      storeRegTemp (m6502_reg_a, true);
    
      m6502_loadRegFromAop (m6502_reg_a, derefaop, offset);
      m6502_emitOp("and", IMMDFMT, (unsigned int)mask);
      m6502_emitRegTempOp("ora", m6502_getLastTempOfs() );
      m6502_storeRegToAop (m6502_reg_a, derefaop, offset);
      pullOrFreeReg (m6502_reg_a, needpulla);
      m6502_loadRegTemp (NULL);
    }
  
  m6502_freeReg (m6502_reg_a);
  
 release:
  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (NULL, derefaop);
}

/**************************************************************************
 * genDataPointerSet - remat pointer to data space
 *************************************************************************/
static void genDataPointerSet (operand * left, operand * right, operand * result, iCode * ic)
{
  int size, offset;
  asmop *derefaop;
  int litOffset = 0;
  char *rematOffset = NULL;

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (right, ic);
  size = AOP_SIZE (right);
  decodePointerOffset (left, &litOffset, &rematOffset);
  wassert (!rematOffset);

  derefaop = aopDerefAop (AOP (result), litOffset);
  m6502_freeAsmop (result, NULL);
  derefaop->size = size;

  if(m6502_findRegAop (AOP(right), 0))
    {
      for(offset=0; offset<size; offset++)
        m6502_transferAopAop (AOP (right), offset, derefaop, offset);
    }
  else
    {
      for(offset=size-1; offset>=0; offset--)
        m6502_transferAopAop (AOP (right), offset, derefaop, offset);
    }

  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (NULL, derefaop);
}

/**************************************************************************
 * genPointerSet - stores the value into a pointer location
 *************************************************************************/
static void
genPointerSet (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *left = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);
  int size, offset;
  bool needloada = false;
  bool needloadx = false;
  bool needloady = false;
  bool restore_a_from_dptr=false;
  bool restore_x_from_dptr=false;
  //bool deadA = false;
  int litOffset = 0;
  char *rematOffset = NULL;
  wassert (operandType (result)->next);
  bool bit_field = IS_BITVAR (operandType (result)->next) && (SPEC_BLEN (operandType (result)->next)%8);

  m6502_emitComment (TRACEGEN, __func__);

  // *(result (reg) + left (rematoffset+litoffset)) = right


  m6502_aopOp (result, ic);

  /* if the result is rematerializable */
  if (AOP_TYPE (result) == AOP_IMMD || AOP_TYPE (result) == AOP_LIT)
    {
      if (!bit_field)
	genDataPointerSet (left, right, result, ic);
      else
	genPackBitsImmed (result, left, operandType (result)->next, right, ic);
      return;
    }

  m6502_aopOp (right, ic);
  //m6502_aopOp (left, ic);

  m6502_printIC(ic);


  size = AOP_SIZE (right);

  decodePointerOffset (left, &litOffset, &rematOffset);

  m6502_emitComment (TRACEGEN|VVDBG, "    %s  - *( reg=%s + litoffset=%d + rematoffset=%s) =",
		     __func__, aopName(AOP(result)),  litOffset, rematOffset );
  m6502_emitComment (TRACEGEN|VVDBG, "                       %s, size=%d",
		     aopName(AOP(right)), size );



  // shortcut for [aa],y (or [aa,x]) if already in zero-page
  // and we're not storing to the same pointer location

  if (!bit_field
      && AOP_TYPE (result) == AOP_DIR && !rematOffset && litOffset >= 0 && litOffset <= 256-size
      && !m6502_sameRegs(AOP(right), AOP(result)) )
    {

#if 0
      if (size == 1 && litOffset == 0 && m6502_reg_x->isLitConst && m6502_reg_x->litConst == 0)
	{
	  // use [aa,x] if only 1 byte and offset is 0
	  m6502_loadRegFromAop (m6502_reg_a, AOP (right), 0);
	  m6502_emitOp ("sta", "[%s,x]", aopAdrStr ( AOP(result), 0, true ) );
	} 
      else
	{ }
#endif
      if(!IS_AOP_A(AOP(right)))
        needloada = storeRegTempIfSurv (m6502_reg_a);

      needloady = storeRegTempIfSurv (m6502_reg_y);
    
      m6502_emitComment (TRACEGEN|VVDBG,"   %s - ptr already in zp ", __func__);
    
#if 0
      if (IS_AOP_XY(AOP(right)))
	{
	  // reverse order so Y is first
	  for (int i=size-1; i>=0; i--)
	    {
	      m6502_loadRegFromAop (m6502_reg_a, AOP (right), i);
	      m6502_loadRegFromConst(m6502_reg_y, litOffset + i);
	      m6502_emitOp ("sta", INDFMT_IY, aopAdrStr ( AOP(result), 0, true ) );
	    }
	}
      else
#endif
	{
	  // forward order
	  for (int i=0; i<size; i++)
	    {
	      m6502_loadRegFromAop (m6502_reg_a, AOP (right), i);
	      m6502_loadRegFromConst(m6502_reg_y, litOffset + i);
	      m6502_emitOp ("sta", INDFMT_IY, aopAdrStr ( AOP(result), 0, true ) );
	    }
	}
      goto release;
    }
  
  // abs,x or abs,y with index in register or memory
  if ( !bit_field 
       && ( AOP_SIZE(result)==1
	    || ( AOP_TYPE(result) == AOP_REG && AOP(result)->aopu.aop_reg[1]->isLitConst ) ) )
    {
      m6502_emitComment (TRACEGEN|VVDBG,"  %s - absolute with 8-bit index", __func__);
        
      m6502_emitComment (TRACEGEN|VVDBG,"    result TYPE=%d",AOP_TYPE (result));
      m6502_emitComment (TRACEGEN|VVDBG,"    AOP(result) reg=%s  size=%d",AOP(result)->aopu.aop_reg[0]->name, AOP_SIZE(result));
      unsigned int hi_offset=0;
      reg_info *src_reg = NULL;
      reg_info *idx_reg = NULL;
      bool px = false;
      bool py = false;
      bool pa = false;
      bool restore_a_from_idx = false;

      if(!rematOffset)
	rematOffset="0x0000";
        
      if(AOP_SIZE(result)==2)
	{
	  hi_offset=(AOP(result)->aopu.aop_reg[1]->litConst)<<8;
	  m6502_freeReg(m6502_reg_x);
	}
        
      //  if ( ( AOP_TYPE(result) == AOP_REG && AOP(result)->aopu.aop_reg[0]->isLitConst ) )
      //      emitcode("ERROR","");
        
        
      if(AOP_TYPE(result)==AOP_REG)
	idx_reg=AOP(result)->aopu.aop_reg[0];
        
      if(AOP_TYPE(right)==AOP_REG)
	src_reg = AOP(right)->aopu.aop_reg[0];

        
      m6502_emitComment (TRACEGEN|VVDBG,"    idx_reg=%s",idx_reg->name);

      if(idx_reg==m6502_reg_a && !m6502_reg_a->isDead)
	restore_a_from_idx=true;
      else
	pa=pushRegIfSurv(m6502_reg_a);
        
      if(idx_reg==m6502_reg_a || idx_reg==NULL)
	{
	  if(src_reg==m6502_reg_x || AOP_TYPE(right)==AOP_SOF)
	    {
	      py = storeRegTempIfSurv(m6502_reg_y);
	      m6502_loadRegFromAop(m6502_reg_y, AOP(result), 0 );
	      idx_reg=m6502_reg_y;
	    }
	  else
	    {
	      px = storeRegTempIfSurv(m6502_reg_x);
	      m6502_loadRegFromAop(m6502_reg_x, AOP(result), 0 );
	      idx_reg=m6502_reg_x;
	    }
	}
        
      if(!px && AOP_TYPE(right)==AOP_SOF)
	px = storeRegTempIfSurv(m6502_reg_x);
        
      for (offset=0; offset<size; offset++)
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (right), offset);        
	  m6502_emitOp("sta", "(%s+0x%04x+%d),%s",
		       rematOffset, hi_offset+litOffset, offset, idx_reg->name );
	}
        
      if(restore_a_from_idx)
	m6502_transferRegReg(idx_reg, m6502_reg_a, true);
      else
	pullOrFreeReg(m6502_reg_a, pa);

      m6502_loadOrFreeRegTemp(m6502_reg_y,py);
      m6502_loadOrFreeRegTemp(m6502_reg_x,px);

        
      goto release;
    }

  // FIXME: one case in qct_0015 stack-auto ends up in general
  // but could be captured in abs,x

  // general case
  m6502_emitComment (TRACEGEN|VVDBG,"  %s - general case ", __func__);
  int aloc=0;
  int xloc=0;
  int yloc=0;

  asmop *ptr_aop = m6502_reg_a->aop;
  char *ptr_str = NULL;
  bool need_x = (AOP_TYPE(right)==AOP_SOF);
  bool use_dptr = true;

  int yoff;

  if(IS_AOP_XA(AOP(result)) && !rematOffset)
    {
      if(ptr_aop && ptr_aop->type==AOP_DIR)
        {
	  use_dptr=false;
        }
      else
        {
          restore_a_from_dptr = !m6502_reg_a->isDead;
          if(need_x)
            restore_x_from_dptr = !m6502_reg_x->isDead;
        }
    }
  else
    {

      if(IS_SAME_DPTR_OP(result) && !rematOffset && litOffset<255)
        {
          // do nothing: no need to save a in this case
          if(!IS_AOP_A(AOP(right)))
            needloada = storeRegTempIfSurv (m6502_reg_a);
          else
            m6502_emitComment (TRACEGEN|VVDBG,"    %s : skip saving A", __func__ );
        }
      else if(AOP_TYPE(result)!=AOP_SOF && IS_AOP_A(AOP(right)) && !rematOffset && litOffset<255)
        {
          // do nothing: no need to save a in this case
          m6502_emitComment (TRACEGEN|VVDBG,"    %s : skip saving A2", __func__ );
        }
      else if(IS_AOP_WITH_A(AOP(right)) && (AOP_TYPE(result)==AOP_SOF))
        needloada = storeRegTempIfUsed (m6502_reg_a);
      else if(IS_AOP_WITH_A(AOP(right)) && !m6502_reg_x->isDead && !m6502_reg_y->isDead)
        needloada = storeRegTempIfUsed (m6502_reg_a);
      else if(IS_AOP_WITH_A(AOP(right)) && IS_AOP_XY(AOP(result)))
        needloada = storeRegTempIfUsed (m6502_reg_a);
      else
        needloada = storeRegTempIfSurv (m6502_reg_a);
      aloc = m6502_getLastTempOfs();

      if(IS_AOP_WITH_X(AOP(right)) && AOP_TYPE(result)==AOP_SOF )
        needloadx = storeRegTempIfUsed (m6502_reg_x);
      else
	//   if(AOP_TYPE(result)==AOP_SOF || AOP_TYPE(right)==AOP_SOF)
        needloadx = storeRegTempIfSurv (m6502_reg_x);
      xloc = m6502_getLastTempOfs();
    } 

  // FIXME: why removing this makes perf worse?
#if 1
  if(IS_AOP_WITH_Y(AOP(right)))
    needloady = storeRegTempIfUsed (m6502_reg_y);
  else
#endif
    needloady = storeRegTempIfSurv (m6502_reg_y);

  yloc = m6502_getLastTempOfs();

  /* if bit-field then pack */
  if (bit_field)
    {
      m6502_emitComment (TRACEGEN|VVDBG,"    %s : bitvar", __func__ );

      if(needloada && IS_AOP_WITH_A (AOP(right)))
	m6502_loadRegTempAt(m6502_reg_a, aloc);
      genPackBits (result, left, operandType (result)->next, right);
      goto release;
    }

#if 0
  bool savea = false;
  if(!m6502_reg_a->isFree)
    {
      savea = true;
      m6502_transferRegReg(m6502_reg_a, m6502_reg_y, true);
    }
#endif

  if(use_dptr)
    {
      ptr_str = "DPTR";
      yoff = setupDPTR(result, litOffset, rematOffset, 
		       !needloada && IS_AOP_WITH_A(AOP(right)) && AOP_TYPE(result)!=AOP_SOF);
    }
  else
    {
      ptr_str = ptr_aop->aopu.aop_dir;
      yoff=litOffset;
    }

  if(IS_AOP_WITH_X(AOP(result)))
    m6502_freeReg(m6502_reg_x);

  if(IS_AOP_WITH_A (AOP(right)))
    if(needloada)
      m6502_loadRegTempAt(m6502_reg_a, aloc);

  if(IS_AOP_WITH_Y (AOP(right)))
    if(needloady)
      m6502_loadRegTempAt(m6502_reg_y, yloc);

  if(IS_AOP_XA (AOP(right)) && m6502_reg_x->isDead)
    {
      m6502_loadRegFromAop (m6502_reg_a, AOP (right), 0);
      m6502_loadRegFromConst(m6502_reg_y, yoff);
      m6502_emitOp("sta", INDFMT_IY, ptr_str);

      if(needloadx)
	m6502_loadRegTempAt(m6502_reg_a, xloc);
      else
	m6502_loadRegFromAop (m6502_reg_a, AOP (right), 1);

      m6502_loadRegFromConst(m6502_reg_y, yoff + 1);
      m6502_emitOp("sta", INDFMT_IY, ptr_str);
    }
  else
    {
      if(IS_AOP_WITH_X (AOP(right)))
        if(needloadx)
	  m6502_loadRegTempAt(m6502_reg_x, xloc);

      // FIXME: optimize for right==AOP_REG
      // can load A directly from the save location
      for (offset=0; offset<size; offset++)
	{
	  m6502_loadRegFromAop (m6502_reg_a, AOP (right), offset);
	  m6502_loadRegFromConst(m6502_reg_y, yoff + offset);
          m6502_emitOp("sta", INDFMT_IY, ptr_str);
        }
    }

 release:
  m6502_loadOrFreeRegTemp (m6502_reg_y, needloady);

  if(!m6502_reg_x->isDead)
    m6502_loadOrFreeRegTemp (m6502_reg_x, needloadx);
  else if(needloadx)
    m6502_loadRegTemp(NULL);

  if(!m6502_reg_a->isDead && !IS_AOP_A (AOP(right)))
    m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
  else if(needloada)
    m6502_loadRegTemp(NULL);

  if(restore_x_from_dptr)
    loadRegFromDPTR(m6502_reg_x, 1);
  if(restore_a_from_dptr)
    loadRegFromDPTR(m6502_reg_a, 0);
    
  m6502_freeAsmop (result, NULL);
  m6502_freeAsmop (right, NULL);    
}

// TODO: genIfx sometimes does a cmp #0 but has flags already, peephole might fix
/**************************************************************************
 * genIfx - generate code for Ifx statement
 *************************************************************************/
static void genIfx (iCode * ic, iCode * popIc)
{
  operand *cond = IC_COND (ic);
  
  m6502_emitComment (TRACEGEN, __func__);
  
  m6502_aopOp (cond, ic);
  
  /* If the condition is a literal, we can just do an unconditional */
  /* branch or no branch */
  if (AOP_TYPE (cond) == AOP_LIT)
    {
      unsigned long long lit = ullFromVal (AOP (cond)->aopu.aop_lit);
      m6502_freeAsmop (cond, NULL);
    
      if (lit)
        {
          if (IC_TRUE (ic))
            m6502_emitBranch ("jmp", IC_TRUE (ic));
        }
      else
        {
          if (IC_FALSE (ic))
            m6502_emitBranch ("jmp", IC_FALSE (ic));
        }
      ic->generated = 1;
      return;
    }

  /* evaluate the operand */
  if (AOP_TYPE (cond) != AOP_CRY)
    {
      m6502_emitComment (TRACEGEN|VVDBG, "     genIfx - !AOP_CRY");
      asmopToBool (AOP (cond), false);
    }
  /* the result is now in the z flag bit */
  m6502_freeAsmop (cond, NULL);

  // TODO: redundant bne/beq
  m6502_emitComment (TRACEGEN|VVDBG, "      genIfx - call jump");
  m6502_genIfxJump (ic, "z");

  ic->generated = 1;
}

/**************************************************************************
 * genAddrOf - generates code for address of
 *************************************************************************/
static void genAddrOf (iCode * ic)
{
  operand *result = IC_RESULT (ic);
  symbol *sym = OP_SYMBOL (IC_LEFT (ic));
  int size, offset;
  bool needloada, needloadx;
  struct dbuf_s dbuf;

  m6502_emitComment (TRACEGEN, "%s - symbol: %s %s",
		     __func__, sym->rname, (sym->onStack)?"(on stack)":"");

  m6502_aopOp (result, ic);

  /* if the operand is on the stack then we
     need to get the stack offset of this
     variable */
  if (sym->onStack)
    {
      needloada = storeRegTempIfSurv (m6502_reg_a);
      needloadx = storeRegTempIfSurv (m6502_reg_x);
      /* if it has an offset then we need to compute it */

      m6502_emitComment (TRACEGEN|VVDBG, "  %s : sym on stack @ %d", __func__, sym->stack);

      if(m6502_reg_x->aop==&m6502_tsxaop)
        {
          int oldOff = -(_S.stackBase - m6502_reg_x->stackOffset + _S.stackPushes + sym->stack +1);
          int newOff = -(_S.stackBase + sym->stack + 1);

          m6502_emitComment (TRACEGEN|VVDBG, "  %s : old: %d   new: %d", __func__, oldOff, newOff);

          if(ABS(newOff)<4 && ABS(newOff)<ABS(oldOff))
            m6502_dirtyReg(m6502_reg_x);
        }

      m6502_emitTSX();
      offset = _S.stackBase - m6502_reg_x->stackOffset + _S.stackPushes + sym->stack + 1;

      if(m6502_smallAdjustReg(m6502_reg_x, offset))
	offset=0;

      m6502_transferRegReg (m6502_reg_x, m6502_reg_a, true);
      if (offset)
	{
	  m6502_emitSetCarry(0);
	  m6502_emitOp ("adc", IMMDFMT, (unsigned int)offset & 0xffu);
	}

      if(IS_AOP_XA(AOP(result)))
	{
          m6502_reg_a->aop=&m6502_tsxaop;
          m6502_reg_a->stackOffset+=offset;
	  m6502_loadRegFromConst(m6502_reg_x, 0x01); // stack top = 0x100
	}
      else
	{
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
	  m6502_loadRegFromConst(m6502_reg_a, 0x01); // stack top = 0x100
	  m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
	}
      m6502_loadOrFreeRegTemp (m6502_reg_x, needloadx);
      m6502_loadOrFreeRegTemp (m6502_reg_a, needloada);
      goto release;
    }

  /* object not on stack then we need the name */
  size = AOP_SIZE (result);
  offset = 0;

  while (size--)
    {
      dbuf_init (&dbuf, 64);
      switch (offset)
	{
	case 0:
	  dbuf_printf (&dbuf, "#%s", sym->rname);
	  break;
	case 1:
	  dbuf_printf (&dbuf, "#>%s", sym->rname);
	  break;
	default:
	  dbuf_printf (&dbuf, "#0");
	}
      storeImmToAop (dbuf_detach_c_str (&dbuf), AOP (result), offset++);
    }

 release:
  m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * genAssignLit - Try to generate code for literal assignment.
 *                result and right should already be asmOped
 *************************************************************************/
static bool genAssignLit (operand * result, operand * right)
{
  char assigned[8];
  unsigned char value[sizeof(assigned)];
  int size;
  int offset,offset2;
  bool restore_a = false;
  bool restore_x = false;

  /* Make sure this is a literal assignment */
  if (AOP_TYPE (right) != AOP_LIT)
    return false;

  /* The general case already handles register assignment well */
  if (AOP_TYPE (result) == AOP_REG)
    return false;

  /* Some hardware registers require MSB to LSB assignment order */
  /* so don't optimize the assignment order if volatile */
  if (isOperandVolatile (result, false))
    return false;

  /* Make sure the assignment is not larger than we can handle */
  size = AOP_SIZE (result);
  if (size > sizeof(assigned))
    return false;

  m6502_emitComment (TRACEGEN, __func__);

  for (offset=0; offset<size; offset++)
    {
      assigned[offset] = 0;
      value[offset] = byteOfVal (AOP (right)->aopu.aop_lit, offset);
    }

  if(AOP_TYPE(result)==AOP_SOF)
    {
      restore_a = storeRegTempIfSurv(m6502_reg_a);
      restore_x = storeRegTempIfSurv(m6502_reg_x);
    }

  m6502_emitComment (TRACEGEN, "  %s - ra:%d rx:%d", __func__, restore_a, restore_x);

  for (offset=0; offset<size; offset++)
    {
      if (assigned[offset])
	continue;
      m6502_storeConstToAop (value[offset], AOP (result), offset);
      assigned[offset] = 1;
      // look for duplicates
      if ((AOP_TYPE (result) != AOP_DIR ))
	{
	  for(offset2=offset+1; offset2<size; offset2++)
	    {
	      if(value[offset]==value[offset2])
		{
		  m6502_storeConstToAop (value[offset], AOP (result), offset2);
		  assigned[offset2] = 1;
		}
	    }
	}
    }

  if(restore_x)
    m6502_loadRegTemp(m6502_reg_x);
  if(restore_a)
    m6502_loadRegTemp(m6502_reg_a);

  return true;
}

/**************************************************************************
 * genAssign - generate code for assignment
 *************************************************************************/
static void
genAssign (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *result = IC_RESULT (ic);

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);
  m6502_printIC(ic);

  if (!genAssignLit (result, right))
    m6502_copy (result, right);

  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * genJumpTab - generates code for jump table
 *************************************************************************/
static void genJumpTab (iCode * ic)
{
  symbol *jtab;
  symbol *jtablo = m6502_safeNewiTempLabel (NULL);
  symbol *jtabhi = m6502_safeNewiTempLabel (NULL);

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (IC_JTCOND (ic), ic);

  // TODO
  {
    bool needpulla = pushRegIfSurv (m6502_reg_a);
    // use X or Y for index?
    bool needpullind = false;
    reg_info* indreg;
    if (IS_AOP_X (AOP (IC_JTCOND (ic))))
      {
	indreg = m6502_reg_x;
      } 
    else if (IS_AOP_Y (AOP (IC_JTCOND (ic))))
      {
	indreg = m6502_reg_y;
      }
    else 
      {
	indreg = m6502_reg_x->isFree ? m6502_reg_x : m6502_reg_y;
	needpullind = pushRegIfSurv (indreg);
	/* get the condition into indreg */
	m6502_loadRegFromAop (indreg, AOP (IC_JTCOND (ic)), 0);
      }
    m6502_freeAsmop (IC_JTCOND (ic), NULL);

    m6502_emitOp ("lda", "%05d$,%s", m6502_safeLabelNum (jtablo), indreg->name);
    storeRegToDPTR(m6502_reg_a, 0);
    m6502_emitOp ("lda", "%05d$,%s", m6502_safeLabelNum (jtabhi), indreg->name);
    storeRegToDPTR(m6502_reg_a, 1);

    if (needpullind)
      m6502_pullReg(indreg);
    if (needpulla)
      m6502_pullReg(m6502_reg_a);

    m6502_emitOp ("jmp", "[DPTR]");

    m6502_dirtyAllRegs();
    m6502_freeAllRegs ();
  }

  /* now generate the jump labels */
  m6502_safeEmitLabel (jtablo);
  // FIXME: add this to gen6502op
  for (jtab = setFirstItem (IC_JTLABELS (ic)); jtab; jtab = setNextItem (IC_JTLABELS (ic)))
    {
      emitcode (".db", "%05d$", labelKey2num (jtab->key));
      regalloc_dry_run_cost_bytes++;
    }
  m6502_safeEmitLabel (jtabhi);
  for (jtab = setFirstItem (IC_JTLABELS (ic)); jtab; jtab = setNextItem (IC_JTLABELS (ic)))
    {
      emitcode (".db", ">%05d$", labelKey2num (jtab->key));
      regalloc_dry_run_cost_bytes++;
    }
}

/**************************************************************************
 * genCast - generate code for casting
 *************************************************************************/
static void genCast (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *result = IC_RESULT (ic);
  sym_link *resulttype = operandType (result);
  sym_link *righttype = operandType (right);
  int size, offset;
  bool signExtend;
  bool save_a = false;

  m6502_emitComment (TRACEGEN, __func__);

  /* if they are equivalent then do nothing */
  if (operandsEqu (result, right))
    return;

  unsigned topbytemask = (IS_BITINT (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8)) ?
    (0xff >> (8 - SPEC_BITINTWIDTH (resulttype) % 8)) : 0xff;

  m6502_aopOp (right, ic);
  m6502_aopOp (result, ic);
  m6502_printIC(ic);

  m6502_emitComment (TRACEGEN|VVDBG, "      genCast - size %d -> %d", right?AOP_SIZE(right):0, result?AOP_SIZE(result):0);

  // Cast to _BitInt can require mask of top byte.
  if (IS_BITINT (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8) && bitsForType (resulttype) < bitsForType (righttype))
    {
      m6502_copy (result, right);
      if (result->aop->type != AOP_REG || result->aop->aopu.aop_reg[result->aop->size - 1] != m6502_reg_a)
	{
	  save_a = result->aop->aopu.aop_reg[0] == m6502_reg_a || !m6502_reg_a->isDead;
	  if (save_a)
 	    m6502_fastSaveA();

	  m6502_loadRegFromAop (m6502_reg_a, result->aop, result->aop->size - 1);
	}
      m6502_emitOp ("and", IMMDFMT, topbytemask);
      if (!SPEC_USIGN (resulttype))
	{
	  // sign extend
	  symbol *tlbl = m6502_safeNewiTempLabel (NULL);
	  m6502_pushReg (m6502_reg_a, true);
	  m6502_emitOp ("and", IMMDFMT, 1u << (SPEC_BITINTWIDTH (resulttype) % 8 - 1));
	  m6502_emitBranch ("beq", tlbl);
	  m6502_pullReg (m6502_reg_a);
	  m6502_emitOp ("ora", IMMDFMT, ~topbytemask & 0xff);
	  m6502_pushReg (m6502_reg_a, true);
	  m6502_safeEmitLabel (tlbl);
	  m6502_pullReg (m6502_reg_a);
	}
      m6502_storeRegToAop (m6502_reg_a, result->aop, result->aop->size - 1);
      goto release;
    }

  if (IS_BOOL (operandType (result)))
    {
      bool needpulla = pushRegIfSurv (m6502_reg_a);
      asmopToBool (AOP (right), true);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
      pullOrFreeReg (m6502_reg_a, needpulla);
      goto release;
    }

  signExtend = AOP_SIZE (result) > AOP_SIZE (right) && !IS_BOOL (righttype) && IS_SPEC (righttype) && !SPEC_USIGN (righttype);
  bool masktopbyte = IS_BITINT (resulttype) && (SPEC_BITINTWIDTH (resulttype) % 8) && SPEC_USIGN (resulttype);
  
  m6502_emitComment (TRACEGEN|VVDBG, "      %s - signExtend: %s",
		     __func__, signExtend?"YES":"NO");

  // if the result size is <= source just copy
  if( (AOP_SIZE (result) <= AOP_SIZE (right))  
      || ((!signExtend) && !IS_AOP_A(AOP(right))) 
      )
    {
      m6502_copy (result, right);
      goto release;
    }

  if(IS_AOP_XA(AOP(right)))
    {
      m6502_storeRegToFullAop (m6502_reg_xa, AOP (result), signExtend);
      goto release;
    }
  
  if(IS_AOP_XY(AOP(right)))
    {
      m6502_storeRegToFullAop (m6502_reg_xy, AOP (result), signExtend);
      goto release;
    }

  if (IS_AOP_XA (AOP (result)) || IS_AOP_XY (AOP (result)) )
    {

      m6502_copy (result, right);
      if (signExtend)
        {
          symbol *tlbl = m6502_safeNewiTempLabel (NULL);
	  m6502_emitCmp(result->aop->aopu.aop_reg[0], 0);
          m6502_emitBranch ("bpl", tlbl);
          m6502_storeConstToAop (0xff, AOP (result), 1);
          m6502_safeEmitLabel (tlbl);
          m6502_dirtyReg(m6502_reg_x);
        }
      goto release;
    }
 
  wassert (AOP (result)->type != AOP_REG);
  
  reg_info *reg = NULL;

  if(!IS_AOP_A(AOP(right)))
    reg=m6502_getFreeByteReg();

  if(!reg 
     && (signExtend|| AOP_TYPE(right)==AOP_SOF || AOP_TYPE(result)==AOP_SOF))
    {
      save_a = fastSaveAIfSurv();
      reg=m6502_reg_a;
    }
  
  offset = 0;
  size = AOP_SIZE (right);

  while (size)
    {
      if (size == 1 && signExtend)
	{
	  m6502_loadRegFromAop (reg, AOP (right), offset);
	  m6502_storeRegToAop (reg, AOP (result), offset);
	  offset++;
	  size--;
	}
      else
	{
	  m6502_transferAopAop (AOP (right), offset, AOP (result), offset);
	  offset++;
	  size--;
	}
    }

  size = AOP_SIZE (result) - offset;
  if (size && !signExtend)
    {
      while (size--)
	m6502_storeConstToAop (0, AOP (result), offset++);
    }
  else if (size)
    {
      m6502_signExtendReg(reg);
      while (size--)
	{
	  if (!size && masktopbyte)
	    m6502_emitOp ("and", IMMDFMT, topbytemask);

	  m6502_storeRegToAop (reg, AOP (result), offset++);
	}
    }

 release:
  fastRestoreOrFreeA(save_a);

  m6502_freeAsmop (right, NULL);
  m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * genReceive - generate code for a receive iCode
 *************************************************************************/
static void
genReceive (iCode * ic)
{
  operand *result = IC_RESULT (ic);
  int size;
  int offset;
  bool delayed_x = false;

  m6502_emitComment (TRACEGEN, __func__);

  m6502_aopOp (result, ic);
  size = AOP_SIZE (result);
  offset = 0;

  m6502_emitComment (TRACEGEN|VVDBG, "  %s: size=%d regmask=%x",
		     __func__, size, AOP (result)->regmask );

  if(ic->argreg)
    {
      if (AOP_TYPE(result)==AOP_SOF && size>1 )
        {
          storeRegTemp (m6502_reg_x, true);
          m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
	  m6502_loadRegTemp (m6502_reg_a);
          m6502_storeRegToAop (m6502_reg_a, AOP (result), 1);
          if(size==3)
            m6502_storeRegToAop (m6502_reg_y, AOP (result), 2);
	}
      else
	{
	  while (size--)
	    {
	      if (AOP_TYPE (result) == AOP_REG && !(offset + (ic->argreg - 1)) 
		  && AOP (result)->aopu.aop_reg[0]->rIdx == X_IDX && size)
		{
		  storeRegTemp (m6502_reg_a, true);
		  delayed_x = true;
		}
	      else
		m6502_transferAopAop (m6502_aop_pass[offset + (ic->argreg - 1)], 0, AOP (result), offset);
	      // FIXME: this freereg is likely wrong
	      if (m6502_aop_pass[offset]->type == AOP_REG)
		m6502_freeReg (m6502_aop_pass[offset]->aopu.aop_reg[0]);
	      offset++;
	    }
	}
    }

  if (delayed_x)
    m6502_loadRegTemp (m6502_reg_x);

  m6502_freeAsmop (result, NULL);
}

// support routine for genDummyRead
static void dummyRead (iCode* ic, operand* op, reg_info* reg)
{
  if (op && IS_SYMOP (op))
    {
      m6502_aopOp (op, ic);
      int size = AOP_SIZE (op);
      for (int offset=0; offset<size; offset++)
        m6502_loadRegFromAop (reg, AOP (op), offset);

      m6502_freeAsmop (op, NULL);
    }
}

/**************************************************************************
 * genDummyRead - generate code for dummy read of volatiles
 *************************************************************************/
static void
genDummyRead (iCode * ic)
{
  bool needpulla = false;
    
  m6502_emitComment (TRACEGEN, __func__);

  reg_info* reg = m6502_getFreeByteReg();
  if (!reg)
    {
      needpulla = pushRegIfSurv (m6502_reg_a);
      reg = m6502_reg_a;
    }

  // TODO: use BIT? STA?
  dummyRead(ic, IC_RIGHT(ic), reg);
  dummyRead(ic, IC_LEFT(ic), reg);

  pullOrFreeReg (reg, needpulla);
}

/**************************************************************************
 * genCritical - generate code for start of a critical sequence
 *************************************************************************/
static void
genCritical (iCode * ic)
{
  operand *result = IC_RESULT (ic);
  m6502_emitComment (TRACEGEN, __func__);
  
  if (result)
    m6502_aopOp (result, ic);

  m6502_emitOp ("php", "");
  m6502_emitOp ("sei", "");

  if (result)
    {
      m6502_emitOp ("plp", "");
      m6502_dirtyReg (m6502_reg_a);
      m6502_storeRegToAop (m6502_reg_a, AOP (result), 0);
    }

  m6502_freeReg (m6502_reg_a);
  if (result)
    m6502_freeAsmop (result, NULL);
}

/**************************************************************************
 * genEndCritical - generate code for end of a critical sequence
 *************************************************************************/
static void
genEndCritical (iCode * ic)
{
  operand *right  = IC_RIGHT (ic);
  m6502_emitComment (TRACEGEN, __func__);
  
  if (right)
    {
      m6502_aopOp (right, ic);
      m6502_loadRegFromAop (m6502_reg_a, AOP (right), 0);
      m6502_emitOp ("pha", "");
      m6502_freeReg (m6502_reg_a);
      m6502_freeAsmop (right, NULL);
    }
  m6502_emitOp ("plp", "");
}

static void
updateiTempRegisterUse (operand * op)
{
  symbol *sym;

  if (IS_ITEMP (op)) {
    sym = OP_SYMBOL (op);
    if (!sym->isspilt)
      {
        /* If only used by IFX, there might not be any register assigned */
        int i;
        for(i = 0; i < sym->nRegs; i++)
          if (sym->regs[i])
            m6502_useReg (sym->regs[i]);
      }
  }
}

/**************************************************************************
 * genm6502iCode - generate code for M6502 based controllers for a
 *                 single iCode instruction
 *************************************************************************/
static void
genm6502iCode (iCode *ic)
{
  operand *right  = IC_RIGHT (ic);
  operand *left   = IC_LEFT (ic);
  operand *result = IC_RESULT (ic);

  int i;

  initGenLineElement ();
  genLine.lineElement.ic = ic;

#if 0
  if (!regalloc_dry_run)
    printf ("ic %d op %d stack pushed %d\n", ic->key, ic->op, G.stack.pushed);
#endif

  if (resultRemat (ic))
    {
      m6502_emitComment(TRACEGEN, "skipping iCode since result will be rematerialized");
      return;
    }

  if (ic->generated)
    {
      m6502_emitComment(TRACEGEN, "skipping generated iCode");
      return;
    }

  m6502_freeAllRegs ();

  // FIXME: removing the following generates worse code
  if (regalloc_dry_run)
    m6502_dirtyAllRegs ();

  if (ic->op == IFX)
    updateiTempRegisterUse (IC_COND (ic));
  else if (ic->op == JUMPTABLE)
    updateiTempRegisterUse (IC_JTCOND (ic));
  else if (ic->op == RECEIVE)
    {
      // FIXME: should add entry icode to this.
      m6502_useReg (m6502_reg_a);
      m6502_useReg (m6502_reg_x); // TODO: x really is free if function only receives 1 byte
    }
  else
    {
      if (POINTER_SET (ic))
	updateiTempRegisterUse (result);
      updateiTempRegisterUse (left);
      updateiTempRegisterUse (right);
    }

  for (i = 0; i < HW_REG_SIZE; i++)
    {
      if (bitVectBitValue (ic->rSurv, i))
        {
          m6502_regWithIdx (i)->isDead = false;
	  m6502_regWithIdx (i)->isFree = false;
        }
      else
	m6502_regWithIdx (i)->isDead = true;
    }

  /* depending on the operation */
  switch (ic->op)
    {
    case '!':
      genNot (ic);
      break;

    case UNARYMINUS:
      genUminus (ic);
      break;

    case IPUSH:
      genIpush (ic);
      break;

    case IPUSH_VALUE_AT_ADDRESS:
      genPointerPush (ic);
      break;

    case CALL:
      genCall (ic);
      break;

    case PCALL:
      genPcall (ic);
      break;

    case FUNCTION:
      genFunction (ic);
      break;

    case ENDFUNCTION:
      genEndFunction (ic);
      break;

    case RETURN:
      genRet (ic);
      break;

    case LABEL:
      genLabel (ic);
      break;

    case GOTO:
      genGoto (ic);
      break;

    case '+':
      m6502_genPlus (ic);
      break;

    case '-':
      m6502_genMinus (ic);
      break;

    case '*':
      genMult (ic);
      break;

    case '/':
      genDiv (ic);
      break;

    case '%':
      genMod (ic);
      break;

    case '>':
    case '<':
    case LE_OP:
    case GE_OP:
      genCmp (ic, ifxForOp (result, ic));
      break;

    case NE_OP:
    case EQ_OP:
      genCmpEQorNE (ic, ifxForOp (result, ic));
      break;

    case AND_OP:
      genAndOp (ic);
      break;

    case OR_OP:
      genOrOp (ic);
      break;

    case '^':
      m6502_genXor (ic, ifxForOp (result, ic));
      break;

    case '|':
      m6502_genOr (ic, ifxForOp (result, ic));
      break;

    case BITWISEAND:
      m6502_genAnd (ic, ifxForOp (result, ic));
      break;

    case INLINEASM:
      m6502_genInline (ic);
      break;

    case GETABIT:
      wassertl (0, "Unimplemented iCode: GETABIT");
      break;

    case GETBYTE:
      genGetByte(ic);
      break;

    case GETWORD:
      genGetWord(ic);
      break;

    case ROT:
      m6502_genRot (ic);
      break;

    case LEFT_OP:
      m6502_genLeftShift (ic);
      break;

    case RIGHT_OP:
      m6502_genRightShift (ic);
      break;

    case GET_VALUE_AT_ADDRESS:
      genPointerGet (ic, NULL); // TODO? ifxForOp (result, ic));
      break;

    case SET_VALUE_AT_ADDRESS:
      genPointerSet (ic);
      break;

    case '=':
      if (POINTER_SET (ic))
        genPointerSet (ic);
      else
        genAssign (ic);
      break;

    case IFX:
      genIfx (ic, NULL);
      break;

    case ADDRESS_OF:
      genAddrOf (ic);
      break;

    case JUMPTABLE:
      genJumpTab (ic);
      break;

    case CAST:
      genCast (ic);
      break;

    case RECEIVE:
      genReceive (ic);
      break;

    case SEND:
      if (!regalloc_dry_run)
        addSet (&_S.sendSet, ic);
      else
        {
	  set * sendSet = NULL;
	  addSet (&sendSet, ic);
	  genSend (sendSet);
	  deleteSet (&sendSet);
        }
      break;

    case DUMMY_READ_VOLATILE:
      genDummyRead (ic);
      break;

    case CRITICAL:
      genCritical (ic);
      break;

    case ENDCRITICAL:
      genEndCritical (ic);
      break;

    default:
      emitcode("ERROR", "; Unimplemented iCode (%x)", ic->op);
      m6502_printIC(ic);
      //      wassertl (0, "Unknown iCode");
    }
}

static void
init_aop_pass(void)
{
  if (m6502_aop_pass[0])
    return;

  m6502_aop_pass[0] = newAsmop (AOP_REG);
  m6502_aop_pass[0]->size = 1;
  m6502_aop_pass[0]->aopu.aop_reg[0] = m6502_reg_a;
  m6502_aop_pass[1] = newAsmop (AOP_REG);
  m6502_aop_pass[1]->size = 1;
  m6502_aop_pass[1]->aopu.aop_reg[0] = m6502_reg_x;
  m6502_aop_pass[2] = newAsmop (AOP_DIR);
  m6502_aop_pass[2]->size = 1;
  m6502_aop_pass[2]->aopu.aop_dir = "___SDCC_m6502_ret2";
  m6502_aop_pass[3] = newAsmop (AOP_DIR);
  m6502_aop_pass[3]->size = 1;
  m6502_aop_pass[3]->aopu.aop_dir = "___SDCC_m6502_ret3";
  m6502_aop_pass[4] = newAsmop (AOP_DIR);
  m6502_aop_pass[4]->size = 1;
  m6502_aop_pass[4]->aopu.aop_dir = "___SDCC_m6502_ret4";
  m6502_aop_pass[5] = newAsmop (AOP_DIR);
  m6502_aop_pass[5]->size = 1;
  m6502_aop_pass[5]->aopu.aop_dir = "___SDCC_m6502_ret5";
  m6502_aop_pass[6] = newAsmop (AOP_DIR);
  m6502_aop_pass[6]->size = 1;
  m6502_aop_pass[6]->aopu.aop_dir = "___SDCC_m6502_ret6";
  m6502_aop_pass[7] = newAsmop (AOP_DIR);
  m6502_aop_pass[7]->size = 1;
  m6502_aop_pass[7]->aopu.aop_dir = "___SDCC_m6502_ret7";
}

float
drym6502iCode (iCode *ic)
{
  regalloc_dry_run = true;
  regalloc_dry_run_cost_bytes = 0;
  regalloc_dry_run_cost_cycles = 0;

  init_aop_pass();

  genm6502iCode (ic);

  destroy_line_list ();
  /*freeTrace (&_S.trace.aops);*/

  wassert (regalloc_dry_run);

  int byte_cost_weight = 1;
  if (optimize.codeSize)
    byte_cost_weight*=4;
  if (!optimize.codeSpeed)
    byte_cost_weight*=2;

  return ((float)regalloc_dry_run_cost_bytes * byte_cost_weight + 4 * regalloc_dry_run_cost_cycles * ic->count);
}

/**************************************************************************
 * genm6502Code - generate code for a block of instructions
 *************************************************************************/
void
genm6502Code (iCode *lic)
{
  iCode *ic;
  int cln = 0;
  int clevel = 0;
  int cblock = 0;

  regalloc_dry_run = false;

  m6502_dirtyAllRegs ();
  _S.tempOfs = 0;

  /* print the allocation information */
  if (allocInfo && currFunc)
    printAllocInfo (currFunc, codeOutBuf);
  /* if debug information required */
  if (options.debug && currFunc && !regalloc_dry_run)
    debugFile->writeFunction (currFunc, lic);

  if (options.debug && !regalloc_dry_run)
    debugFile->writeFrameAddress (NULL, NULL, 0); /* have no idea where frame is now */

  init_aop_pass();

  /* Generate Code for all instructions */
  for (ic = lic; ic; ic = ic->next)
    {
      initGenLineElement ();

      genLine.lineElement.ic = ic;

      if (ic->level != clevel || ic->block != cblock)
	{
	  if (options.debug)
	    debugFile->writeScope (ic);
	  clevel = ic->level;
	  cblock = ic->block;
	}

      if (ic->lineno && cln != ic->lineno)
	{
	  if (options.debug)
	    debugFile->writeCLine (ic);

	  if (!options.noCcodeInAsm)
	    m6502_emitComment (ALWAYS, "%s: %d: %s", ic->filename, ic->lineno, printCLine (ic->filename, ic->lineno));
	  cln = ic->lineno;
	}

      regalloc_dry_run_cost_bytes = 0;
      regalloc_dry_run_cost_cycles = 0;

      if (options.iCodeInAsm)
	{
	  char regsSurv[4];
	  const char *iLine;

	  regsSurv[0] = (bitVectBitValue (ic->rSurv, A_IDX)) ? 'a' : '-';
	  regsSurv[1] = (bitVectBitValue (ic->rSurv, Y_IDX)) ? 'y' : '-';
	  regsSurv[2] = (bitVectBitValue (ic->rSurv, X_IDX)) ? 'x' : '-';
	  regsSurv[3] = 0;
	  iLine = printILine (ic);
	  m6502_emitComment (ALWAYS, " [%s] ic:%d: %s", regsSurv, ic->key, iLine);
	  dbuf_free (iLine);
	}

      genm6502iCode(ic);
      m6502_emitComment (TRACEGEN, "Raw cost for generated ic %d : (%d, %f) count=%f", ic->key, regalloc_dry_run_cost_bytes, regalloc_dry_run_cost_cycles, ic->count);

      // TODO: should be asserts?
#if 1
      if (!m6502_reg_a->isFree)
	m6502_emitComment (REGOPS|VVDBG, "  forgot to free a");
      if (!m6502_reg_x->isFree)
	m6502_emitComment (REGOPS|VVDBG, "  forgot to free x");
      if (!m6502_reg_y->isFree)
	m6502_emitComment (REGOPS|VVDBG, "  forgot to free y");
      if (!m6502_reg_xy->isFree)
	m6502_emitComment (REGOPS|VVDBG, "  forgot to free xy");
      if (!m6502_reg_xa->isFree)
	m6502_emitComment (REGOPS|VVDBG, "  forgot to free xa");
#endif

      if (m6502_getLastTempOfs() != -1 )
	emitcode("ERROR", "; forgot to free temp stack (%d)", m6502_getLastTempOfs());
    }

  if (options.debug)
    debugFile->writeFrameAddress (NULL, NULL, 0); /* have no idea where frame is now */

  /* now we are ready to call the
     peep hole optimizer */
  if (!options.nopeep)
    peepHole (&genLine.lineHead);

  /* now do the actual printing */
  printLine (genLine.lineHead, codeOutBuf);

  /* destroy the line list */
  destroy_line_list ();
}

