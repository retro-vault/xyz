/*-------------------------------------------------------------------------
  gen.h - header file for code generation for mos6502

  Copyright (C) 1998, Sandeep Dutta . sandeep.dutta@usa.net
  Copyright (C) 2026, Gabriele Gorla

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

#ifndef SDCCGENM6502_H
#define SDCCGENM6502_H

// keep this in sync with __sdcc_regs.s in the library
#define NUM_TEMP_REGS 6

enum debug_messages
  {
    ALWAYS=0x01,
    VASM=0x02,
    TRACEGEN=0x04,
    DEEPTRACE=0x08,
    COST=0x10,
    REGALLOC=0x20,
    REGOPS=0x40,
    TRACE_AOP=0x80,
    TRACE_STACK=0x100,
    VVDBG=0x80000000,
    DEBUG_ALL=0x7fffffff
  };

//#define DBG_MSG (REGALLOC)
#define DBG_MSG (REGALLOC|TRACEGEN/*|COST*/)
//#define DBG_MSG (REGALLOC|TRACEGEN|REGOPS/*|COST*/)
//#define DBG_MSG (REGALLOC|TRACEGEN|TRACE_STACK/*|COST*/)
//#define DBG_MSG (DEBUG_ALL/*|VVDBG*/)
//#define DBG_MSG ((DEBUG_ALL|VVDBG)&~COST)

#define DEBUG_UNIMPLEMENTED

#define LSB     0
#define MSB16   1
#define MSB24   2
#define MSB32   3

#define AOP(op) op->aop
#define AOP_TYPE(op) AOP(op)->type
#define AOP_SIZE(op) AOP(op)->size
#define AOP_OP(aop) aop->op

#define IS_AOP_A(x) ((x)->regmask == M6502MASK_A)
#define IS_AOP_X(x) ((x)->regmask == M6502MASK_X)
#define IS_AOP_Y(x) ((x)->regmask == M6502MASK_Y)
#define IS_AOP_XA(x) ((x)->regmask == M6502MASK_XA)
#define IS_AOP_XY(x) ((x)->regmask == M6502MASK_XY)

#define IS_AOP_WITH_REG(x, reg) (((x)->regmask & reg->mask) != 0)
#define IS_AOP_WITH_A(x) (((x)->regmask & M6502MASK_A) != 0)
#define IS_AOP_WITH_X(x) (((x)->regmask & M6502MASK_X) != 0)
#define IS_AOP_WITH_Y(x) (((x)->regmask & M6502MASK_Y) != 0)

typedef enum
  {
    AOP_INVALID,
    AOP_LIT = 1,   /* operand is a literal value */
    AOP_REG,       /* is in registers */
    AOP_DIR,       /* operand using direct addressing mode */
    AOP_STK,       /* should be pushed on stack this
		      can happen only for the result */
    AOP_IMMD,      /* immediate value for eg. remateriazable */
    AOP_CRY,       /* carry contains the value of this */
    AOP_EXT,       /* operand using extended addressing mode */
    AOP_SOF,       /* operand at an offset on the stack */
    AOP_DUMMY,     /* Read undefined, discard writes */
    AOP_IDX        /* operand using indexed addressing mode */
  }
  AOP_TYPE;

/* asmop: A homogenised type for all the different
   spaces an operand can be in */
typedef struct asmop
{
  AOP_TYPE type;		
  short coff;			/* current offset */
  short size;			/* total size */    	
  short regmask;              /* register mask if AOP_REG */
  operand *op;		/* originating operand */
  unsigned freed:1;		/* already freed    */
  unsigned stacked:1;		/* partial results stored on stack */
  struct asmop *stk_aop[4];	/* asmops for the results on the stack */
  union
  {
    value *aop_lit;		/* if literal */
    reg_info *aop_reg[4];	/* array of registers */
    char *aop_dir;		/* if direct/ext  */
    char *aop_immd;         /* if immediate */
    int aop_stk;		/* stack offset when AOP_STK */
  } aopu;
}
  asmop;

struct attr_t
{
  bool isLiteral;
  unsigned char literalValue;
  struct asmop *aop;		/* last operand */
  int aopofs;			/* last operand offset */
  int stackOffset;              /* stack offset when aop==m6502_tsxaop */
};

struct m6502_state_t
{
  int stackBase;
  int funcHasBasePtr;
  int stackPushes;
  //  int baseStackPushes;
  set *sendSet;
  int tempOfs;
  unsigned carryValid:1;
  unsigned carry:1;
  int lastflag;
  struct attr_t tempAttr[NUM_TEMP_REGS];
  struct attr_t DPTRAttr[2];
};

struct binst_t
{
  char flag[4];
  char set[4];
  char clear[4];
};

// globals
extern asmop m6502_tsxaop;
extern unsigned fReturnSizeM6502;
extern bool m6502_assignment_optimal;
extern struct m6502_state_t _S;

extern const char *IMMDFMT; // = "#0x%02x";
extern const char *TEMPFMT_IND; // = "[REGTEMP+%d]";
extern const char *IDXFMT_X; // = "0x%x,x";
//extern char *TEMPFMT_IX; // = "[(REGTEMP+%d),x]";

extern const char *TEMPFMT; // = "*(REGTEMP+%d)";
extern const char *DPTRFMT; // = "*(DPTR+%d)";
extern const char *INDFMT_IY; // = "[%s],y";

extern const int STACK_TOP; // = 0x100;

// utility functions
const char * m6502_regInfoStr();
void m6502_printIC(iCode *ic);
int m6502_isLiteralBit (unsigned long long lit);
unsigned long long m6502_litmask (int size);
void m6502_signExtendReg(reg_info *reg);
//void m6502_addSign (operand * result, int offset, int sign);
bool m6502_smallAdjustReg (reg_info *reg, int n);
void m6502_unimplemented(const char *msg);
void m6502_updateCFA (void);
bool m6502_aopCanIncDec (asmop *aop);
bool m6502_aopCanBit (asmop *aop);
bool m6502_aopCanShift (asmop * aop);
bool m6502_sameRegs (asmop *aop1, asmop *aop2);
reg_info* m6502_getDeadByteReg();
reg_info* m6502_getFreeByteReg();
reg_info* m6502_getFreeIdxReg();
void m6502_copy (operand * result, operand * source);


// code emit
void m6502_emitDebuggerSymbol (const char *);
void m6502_emitOp (const char *inst, const char *fmt, ...);
void m6502_emitComment (unsigned int level, const char *fmt, ...);
bool m6502_emitCmp (reg_info *reg, unsigned char v);
void m6502_emitBranch (const char *branchop, symbol * tlbl);
void m6502_emitTSX(void);
void m6502_emitSetCarry(int c);

// labels
symbol * m6502_safeNewiTempLabel(const char * a);
void m6502_safeEmitLabel(symbol * a);
int m6502_safeLabelNum(symbol * a);

void m6502_transferRegReg (reg_info *sreg, reg_info *dreg, bool freesrc);
void m6502_loadRegFromConst (reg_info * reg, int c);
void m6502_loadRegFromAop (reg_info * reg, asmop * aop, int loffset);
void m6502_storeRegToFullAop (reg_info *reg, asmop *aop, bool isSigned);
void m6502_rmwWithReg (char *rmwop, reg_info * reg);
reg_info* m6502_findRegAop (asmop * aop, int loffset);

void m6502_storeConstToAop (int c, asmop * aop, int loffset);
void m6502_transferAopAop (asmop * srcaop, int srcofs, asmop * dstaop, int dstofs);
void m6502_storeRegToAop (reg_info *reg, asmop * aop, int loffset);
void m6502_accopWithAop (const char *accop, asmop *aop, int loffset);
void m6502_rmwWithAop (char *rmwop, asmop * aop, int loffset);
void m6502_aopOp (operand *op, const iCode * ic);
void m6502_freeAsmop (operand * op, asmop * aaop);

// stack
bool m6502_pushReg (reg_info * reg, bool freereg);
void m6502_pullReg (reg_info * reg);
void m6502_adjustStack (int n); // candidate for moving back into gen.c

#define pushRegIfUsed(r)     (!r->isFree)?m6502_pushReg(r,true):false
#define pushRegIfSurv(r)     (!r->isDead)?m6502_pushReg(r,true):false
#define pullOrFreeReg(r,np)  (np)?m6502_pullReg(r):m6502_freeReg(r)
#define pullNull(n)          m6502_adjustStack(n)

// regtemp
bool m6502_fastSaveAi(reg_info *reg);
bool m6502_fastSaveA();
bool m6502_fastRestoreA();

#define fastSaveAIfUsed()     (!m6502_reg_a->isFree)?m6502_fastSaveA():false
#define fastSaveAIfSurv()     (!m6502_reg_a->isDead)?m6502_fastSaveA():false
#define fastRestoreOrFreeA(np)  (np)?m6502_fastRestoreA():false

bool m6502_storeRegTempi(reg_info * reg, bool freereg, bool force);
#define storeRegTemp(reg,freereg)       m6502_storeRegTempi (reg,freereg,false)
#define storeRegTempAlways(reg,freereg) m6502_storeRegTempi (reg,freereg,true)
#define storeRegTempIfUsed(r)           (!r->isFree)?storeRegTemp(r,true):false
#define storeRegTempIfSurv(r)           (!r->isDead)?storeRegTemp(r,true):false
void m6502_loadRegTempAt (reg_info * reg, int offset);
void m6502_loadRegTemp (reg_info * reg);
void m6502_loadOrFreeRegTemp (reg_info * reg, bool needload);
void m6502_loadRegTempNoFlags (reg_info * reg, bool needload);
void m6502_emitRegTempOp(const char *op, int offset);
void m6502_dirtyRegTemp (int temp_reg_idx);
int  m6502_getLastTempOfs();

// gen functions
//void m6502_genCode (iCode *ic);
void m6502_genIfxJump (iCode * ic, char *jval);
void m6502_genOr (iCode * ic, iCode * ifx);
void m6502_genXor (iCode * ic, iCode * ifx);
void m6502_genAnd (iCode * ic, iCode * ifx);
void m6502_genPlus (iCode * ic);
void m6502_genMinus (iCode * ic);
void m6502_genRot (iCode * ic);

// shifts
void m6502_AccRsh (int shCount, bool sign);
void m6502_AccLsh (int shCount);
void m6502_genRightShift (iCode * ic);
void m6502_genLeftShift (iCode * ic);

#endif

