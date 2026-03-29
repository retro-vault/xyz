/**
 * Z80 code generation header.
 *
 * Defines the `asmop` structure used to represent operands in various Z80-specific
 * addressing modes, such as registers, stack, literals, and indirect memory.
 *
 * Declares key Z80 backend functions and state flags used during code generation
 * and optimization phases.
 *
 * Originally written for 8051 and adapted for Z80 by the SDCC team.
 *
 * Copyright (C) 1998 Sandeep Dutta
 * Released under the GNU General Public License v2 or later.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum
  {
    AOP_INVALID,
    AOP_LIT,     /* Literal value */
    AOP_REG,     /* In register(s) */
    AOP_DIR,     /* Direct memory */
    AOP_SFR,     /* Special function register */
    AOP_STK,     /* Stack location */
    AOP_IMMD,    /* Immediate value */
    AOP_STL,     /* Stack address (reference) */
    AOP_CRY,     /* Carry flag */
    AOP_IY,      /* Indirect via IY */
    AOP_HL,      /* Indirect via HL */
    AOP_EXSTK,   /* Extended stack (IY/HL offset) */
    AOP_PAIRPTR, /* Register pair pointer */
    AOP_DUMMY    /* Discarded/undefined access */
  } AOP_TYPE;

  /* Operand representation for instruction selection */
  typedef struct asmop
  {
    AOP_TYPE type;
    short coff;           /* Offset in memory/register */
    short size;           /* Size in bytes */
    unsigned code : 1;    /* Code space access */
    unsigned paged : 1;   /* Paged memory space */
    unsigned freed : 1;   /* Already freed (dead) */
    unsigned bcInUse : 1; /* BC used for indirect I/O */

    union
    {
      value *aop_lit;
      reg_info *aop_reg[4];
      char *aop_dir;
      char *aop_immd;
      int aop_stk;
      int aop_pairId;
    } aopu;

    signed char regs[9];    /* Byte-to-register mapping, -1 if unused */
    struct valinfo valinfo; /* Cached value info */
  } asmop;

  /* Z80 backend entry point */
  void genZ80Code(iCode *);

  /* Emit debug symbol */
  void z80_emitDebuggerSymbol(const char *);

  /* Check if an expression is returned by function */
  bool z80IsReturned(const char *what);

  /* Check if an expression is part of the ith register argument */
  bool z80IsRegArg(struct sym_link *ftype, int i, const char *what);

  /* Check if an expression is part of any function argument */
  bool z80IsParmInCall(struct sym_link *ftype, const char *what);

  extern bool z80_assignment_optimal;
  extern bool should_omit_frame_ptr;

#ifdef __cplusplus
}
#endif
