/*-------------------------------------------------------------------------
  peep.c - source file for peephole optimizer helper functions

  Copyright (C) 2011-2025, Philipp Klaus Krause pkk@spth.de, philipp@informatik.uni-frankfurt.de, philipp@colecovision.eu
  Copyright (C) 2020, Sebastian 'basxto' Riedel <sdcc@basxto.de>

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

  In other words, you are welcome to use, share and improve this program.
  You are forbidden to forbid anyone else to use, share and improve
  what you give them.   Help stamp out software-hoarding!
-------------------------------------------------------------------------*/

#include "common.h"
#include "SDCCicode.h"
#include "z80.h"
#include "SDCCglobl.h"
#include "SDCCpeeph.h"
#include "gen.h"

#define NOTUSEDERROR() do {werror(E_INTERNAL_ERROR, __FILE__, __LINE__, "error in notUsed()");} while(0)

#if 0
#define D(_s) { printf _s; fflush(stdout); }
#else
#define D(_s)
#endif

typedef enum
{
  S4O_CONDJMP,
  S4O_WR_OP,
  S4O_RD_OP,
  S4O_TERM,
  S4O_VISITED,
  S4O_ABORT,
  S4O_CONTINUE
} S4O_RET;

static struct
{
  lineNode *head;
} _G;

extern bool z80_regs_used_as_parms_in_calls_from_current_function[IYH_IDX + 1];
extern bool z80_symmParm_in_calls_from_current_function;
extern bool z80_regs_preserved_in_calls_from_current_function[IYH_IDX + 1];

/*-----------------------------------------------------------------*/
/* univisitLines - clear "visited" flag in all lines               */
/*-----------------------------------------------------------------*/
static void
unvisitLines (lineNode *pl)
{
  for (; pl; pl = pl->next)
    pl->visited = FALSE;
}

#define AOP(op) op->aop
#define AOP_SIZE(op) AOP(op)->size

/*-----------------------------------------------------------------*/
/* incLabelJmpToCount - increment counter "jmpToCount" in entry    */
/* of the list labelHash                                           */
/*-----------------------------------------------------------------*/
static bool
incLabelJmpToCount (const char *label)
{
  labelHashEntry *entry;

  entry = getLabelRef (label, _G.head);
  if (!entry)
    return FALSE;
  entry->jmpToCount++;
  return TRUE;
}

/*-----------------------------------------------------------------*/
/* findLabel -                                                     */
/* 1. extracts label in the opcode pl                              */
/* 2. increment "label jump-to count" in labelHash                 */
/* 3. search lineNode with label definition and return it          */
/*-----------------------------------------------------------------*/
static lineNode *
findLabel (const lineNode *pl)
{
  char *p;
  lineNode *cpl;

  /* 1. extract label in opcode */

  /* In each z80 jumping opcode the label is at the end of the opcode */
  p = strlen (pl->line) - 1 + pl->line;

  /* scan backward until ',' or '\t' */
  for (; p > pl->line; p--)
    if (*p == ',' || isspace(*p))
      break;

  /* sanity check */
  if (p == pl->line)
    {
      NOTUSEDERROR();
      return NULL;
    }

  /* skip ',' resp. '\t' */
  ++p;

  /* 2. increment "label jump-to count" */
  if (!incLabelJmpToCount (p))
    return NULL;

  /* 3. search lineNode with label definition and return it */
  for (cpl = _G.head; cpl; cpl = cpl->next)
    if (cpl->isLabel &&
      strncmp (p, cpl->line, strlen(p)) == 0 &&
      cpl->line[strlen(p)] == ':')
        return cpl;

  return NULL;
}

/* Check if reading arg implies reading what. */
static bool argCont (const char *arg, const char *what)
{
  wassert (arg);

  while(isspace (*arg) || *arg == ',')
    arg++;

  if (arg[0] == '#' || arg[0] == '_')
    return false;

  if(arg[0] == '(' && arg[1] && arg[2] && (arg[2] != ')' && arg[3] != ')')
     && !(IS_SM83 && (arg[3] == '-' || arg[3] == '+') && arg[4] == ')'))
    return false;

  if(*arg == '(')
    arg++;

  if (arg[0] == '#' || arg[0] == '_')
    return false;
    
  // Get suitable end to avoid reading into further arguments.
  const char *end = strchr(arg, ',');
  if (!end)
    end = arg + strlen(arg);

  const char *found = StrStr(arg, what);

  return(found && found < end);
}

static bool
z80MightBeParmInCallFromCurrentFunction(const char *what)
{
  if (strchr(what, 'l') && z80_regs_used_as_parms_in_calls_from_current_function[L_IDX])
    return TRUE;
  if (strchr(what, 'h') && z80_regs_used_as_parms_in_calls_from_current_function[H_IDX])
    return TRUE;
  if (strchr(what, 'e') && z80_regs_used_as_parms_in_calls_from_current_function[E_IDX])
    return TRUE;
  if (strchr(what, 'd') && z80_regs_used_as_parms_in_calls_from_current_function[D_IDX])
    return TRUE;
  if (strchr(what, 'c') && z80_regs_used_as_parms_in_calls_from_current_function[C_IDX])
    return TRUE;
  if (strchr(what, 'b') && z80_regs_used_as_parms_in_calls_from_current_function[B_IDX])
    return TRUE;
  if (strchr(what, 'a') && z80_regs_used_as_parms_in_calls_from_current_function[A_IDX])
    return true;
  if (strstr(what, "iy") && (z80_regs_used_as_parms_in_calls_from_current_function[IYL_IDX] || z80_regs_used_as_parms_in_calls_from_current_function[IYH_IDX]))
    return true;

  return false;
}

/* Check if the flag implies reading what. */
static bool
z80MightReadFlagCondition(const char *cond, const char *what)
{
  while(isspace (*cond))
    cond++;

  if(!STRNCASECMP(cond, "po", 2) || !STRNCASECMP(cond, "pe", 2))
    return !strcmp(what, "pf");
  if(tolower(cond[0]) == 'm' || tolower(cond[0]) == 'p')
    return !strcmp(what, "sf");

  // skip inverted conditions
  if(tolower(cond[0]) == 'n')
    cond++;

  if(tolower(cond[0]) == 'c')
    return !strcmp(what, "cf");
  if(tolower(cond[0]) == 'z')
    return !strcmp(what, "zf");
  return true;
}

static bool
z80MightReadFlag(const lineNode *pl, const char *what)
{
  if(lineIsInst (pl, "ld") ||
     lineIsInst (pl, "or") ||
     lineIsInst (pl, "cp") ||
     lineIsInst (pl, "di") ||
     lineIsInst (pl, "ei") ||
     lineIsInst (pl, "im") ||
     lineIsInst (pl, "in"))
    return false;
  if(lineIsInst (pl, "nop") ||
     lineIsInst (pl, "add") ||
     lineIsInst (pl, "sub") ||
     lineIsInst (pl, "and") ||
     lineIsInst (pl, "xor") ||
     lineIsInst (pl, "dec") ||
     lineIsInst (pl, "inc") ||
     lineIsInst (pl, "cpl") ||
     lineIsInst (pl, "bit") ||
     lineIsInst (pl, "res") ||
     lineIsInst (pl, "set") ||
     lineIsInst (pl, "pop") ||
     lineIsInst (pl, "rlc") ||
     lineIsInst (pl, "rrc") ||
     lineIsInst (pl, "sla") ||
     lineIsInst (pl, "sra") ||
     lineIsInst (pl, "srl") ||
     lineIsInst (pl, "scf") ||
     lineIsInst (pl, "cpd") ||
     lineIsInst (pl, "cpi") ||
     lineIsInst (pl, "ind") ||
     lineIsInst (pl, "ini") ||
     lineIsInst (pl, "ldd") ||
     lineIsInst (pl, "ldi") ||
     lineIsInst (pl, "neg") ||
     lineIsInst (pl, "rld") ||
     lineIsInst (pl, "rrd") ||
     lineIsInst (pl, "mlt") ||
     lineIsInst (pl, "out"))
    return false;
  if(lineIsInst (pl, "halt") ||
     lineIsInst (pl, "rlca") ||
     lineIsInst (pl, "rrca") ||
     lineIsInst (pl, "cpdr") ||
     lineIsInst (pl, "cpir") ||
     lineIsInst (pl, "indr") ||
     lineIsInst (pl, "inir") ||
     lineIsInst (pl, "lddr") ||
     lineIsInst (pl, "ldir") ||
     lineIsInst (pl, "outd") ||
     lineIsInst (pl, "outi") ||
     lineIsInst (pl, "djnz"))
    return false;

  if ((IS_Z180 || IS_EZ80) &&
    (lineIsInst (pl, "in0") || lineIsInst (pl, "out0") ||
    lineIsInst (pl, "otim") || lineIsInst (pl, "otimr") ||
    lineIsInst (pl, "otdm") || lineIsInst (pl, "otdmr")))
    return(false);

  if(IS_RAB &&
     (lineIsInst (pl, "bool") ||
     lineIsInst (pl, "ldf") ||
     lineIsInst (pl, "ldp") ||
     lineIsInst (pl, "mul")))
    return false;

  if((IS_R3KA || IS_R4K || IS_R5K || IS_R6K) &&
     (lineIsInst (pl, "lsddr") ||
     lineIsInst (pl, "lsidr")))
    return false;

  if (IS_R6K && lineIsInst (pl, "swap"))
    return false;

  if(IS_SM83 &&
    (lineIsInst (pl, "stop") ||
    lineIsInst (pl, "ldh")))
    return false;

  if((IS_SM83 || IS_Z80N) &&
      lineIsInst (pl, "swap"))
    return false;

  if(IS_R800 &&
     (lineIsInst (pl, "multu") ||
     lineIsInst (pl, "multuw")))
    return false;

  if(IS_EZ80 &&
    (lineIsInst (pl, "lea") ||
    lineIsInst (pl, "pea")))
    return false;

  if((IS_R4K || IS_R5K || IS_R6K) &&
    (lineIsInst (pl, "cbm") ||
    lineIsInst (pl, "clr") ||
    lineIsInst (pl, "mulu") ||
    lineIsInst (pl, "test")))
    return false;

  if (IS_R6K && lineIsInst (pl, "swap"))
    return false;

  if(IS_TLCS90 &&
    (lineIsInst (pl, "div") ||
    lineIsInst (pl, "lda") ||
    lineIsInst (pl, "mul")))
    return false;

  if(IS_TLCS90 &&
    (lineIsInst (pl, "decx") ||
    lineIsInst (pl, "incx")))
    return (!strcmp(what, "xf"));

  if(lineIsInst (pl, "rl") ||
     lineIsInst (pl, "rr") ||
     lineIsInst (pl, "rla") ||
     lineIsInst (pl, "rra") ||
     lineIsInst (pl, "sbc") ||
     lineIsInst (pl, "adc") ||
     lineIsInst (pl, "ccf"))
    return (!strcmp(what, "cf"));

  if(lineIsInst (pl, "daa"))
    return (!strcmp(what, "cf") || !strcmp(what, "nf") ||
            !strcmp(what, "hf"));

  if(lineIsInst (pl, "push"))
    return (argCont(pl->line + 4, "af"));

  if(lineIsInst (pl, "ex"))
    return (argCont(pl->line + 2, "af"));

  // catch c, nc, z, nz, po, pe, p and m
  if(lineIsInst (pl, "jp") ||
     lineIsInst (pl, "jr"))
    return (strchr(pl->line, ',') && z80MightReadFlagCondition(pl->line + 2, what));

  // flags don't matter according to calling convention
  if(lineIsInst (pl, "reti") ||
     lineIsInst (pl, "retn"))
    return false;

  if(lineIsInst (pl, "call"))
    return (strchr(pl->line, ',') && z80MightReadFlagCondition(pl->line + 4, what));

  if(lineIsInst (pl, "ret"))
    return (pl->line[3] == '\t' && z80MightReadFlagCondition(pl->line + 3, what));

  // we don't know anything about this
  if(lineIsInst (pl, "rst"))
    return true;

  if (IS_SM83 && 
    (lineIsInst (pl, "ldh") ||
    lineIsInst (pl, "ldhl")))
    return false;
    
  if(IS_RAB && (lineIsInst (pl, "ioi") || lineIsInst (pl, "ioe") || lineIsInst (pl, "ipres")))
    return(false);

  if(IS_Z80N &&
    (lineIsInst (pl, "bsla") ||
    lineIsInst (pl, "bsra") ||
    lineIsInst (pl, "bsrl") ||
    lineIsInst (pl, "bsrf") ||
    lineIsInst (pl, "brlc")))
    return(false);

  if((IS_Z180 || IS_EZ80 || IS_Z80N) &&
    lineIsInst (pl, "tst"))
    return(false);

  if(IS_EZ80 &&
    (lineIsInst (pl, "add.l") ||
    lineIsInst (pl, "dec.l") ||
    lineIsInst (pl, "inc.l") ||
    lineIsInst (pl, "ld.il") ||
    lineIsInst (pl, "ld.l") ||
    lineIsInst (pl, "ld.lil") ||
    lineIsInst (pl, "ld.lis") ||
    lineIsInst (pl, "pop.l") ||
    lineIsInst (pl, "res.l")||
    lineIsInst (pl, "set.l")))
    return(false);

  if(IS_EZ80 && lineIsInst (pl, "push.l"))
    return (argCont(pl->line + 6, "af"));

  printf("z80MightReadFlag unknown asm inst line: %s\n", pl->line);

  return true; // Fail-safe: we have no idea what happens at this line, so assume it might read anything.
}

static bool
z80MightRead(const lineNode *pl, const char *what)
{
  if(strcmp(what, "iyl") == 0 || strcmp(what, "iyh") == 0)
    what = "iy";
  if(strcmp(what, "ixl") == 0 || strcmp(what, "ixh") == 0)
    what = "ix";

  const char *larg = lineArg (pl, 0);
  const char *rarg = lineArg (pl, 1);

  if(lineIsInst (pl, "call") && strcmp(what, "sp") == 0)
    return TRUE;

  if(strcmp(pl->line, "call\t__initrleblock") == 0 && (strchr(what, 'd') != 0 || strchr(what, 'e') != 0))
    return TRUE;

  if((strcmp(pl->line, "call\t___sdcc_call_hl") == 0 || larg && !strncmp (larg, "(hl)", 4)) && (strchr(what, 'h') != 0 || strchr(what, 'l') != 0))
    return true;

  else if((strcmp(pl->line, "call\t___sdcc_call_iy") == 0 || larg && !strncmp (larg, "(iy)", 4)) && strstr(what, "iy") != 0)
    return true;

  if(strncmp(pl->line, "call\t___sdcc_bcall_", 19) == 0)
    if (strchr (what, pl->line[19]) != 0 || strchr (what, pl->line[20]) != 0 || strchr (what, pl->line[21]) != 0)
      return TRUE;

  if(lineIsInst (pl, "call") && strchr(pl->line, ',') == 0)
    {
      const symbol *f = findSym (SymbolTab, 0, pl->line + 6);
      if (f && IS_FUNC (f->type))
        return z80IsParmInCall(f->type, what);
      else // Fallback needed for calls through function pointers and for calls to literal addresses.
        return z80MightBeParmInCallFromCurrentFunction(what);
    }

  if(lineIsInst (pl, "reti") || lineIsInst (pl, "retn"))
    return(strcmp(what, "sp") == 0);

  if(lineIsInst (pl, "ret")) // --reserve-regs-iy and the sm83 port use ret in code gen for calls through function pointers.
    return((IY_RESERVED || IS_SM83) ? z80IsReturned(what) || z80MightBeParmInCallFromCurrentFunction(what) : z80IsReturned(what)) || strcmp(what, "sp") == 0;
  if(IS_RAB && (lineIsInst (pl, "lret") || lineIsInst (pl, "llret"))) // So do the Rabbit ports with lret and llret.
    return(z80IsReturned(what) || z80MightBeParmInCallFromCurrentFunction(what));

  if(!strcmp(pl->line, "ex\t(sp), hl") || !strcmp(pl->line, "ex\t(sp),hl"))
    return(!strcmp(what, "h") || !strcmp(what, "l") || strcmp(what, "sp") == 0);
  if(!strcmp(pl->line, "ex\t(sp), ix") || !strcmp(pl->line, "ex\t(sp),ix"))
    return(!!strstr(what, "ix") || strcmp(what, "sp") == 0);
  if(!strcmp(pl->line, "ex\t(sp), iy") || !strcmp(pl->line, "ex\t(sp),iy"))
    return(!!strstr(what, "iy") || strcmp(what, "sp") == 0);
  if(!strcmp(pl->line, "ex\tde, hl") || !strcmp(pl->line, "ex\tde,hl"))
    return(!strcmp(what, "h") || !strcmp(what, "l") || !strcmp(what, "d") || !strcmp(what, "e"));
  if(IS_RAB && lineIsInst (pl, "ldp") && !strcmp(what, "a")) // All ldp use the value in a.
    return(true);
  if(lineIsInst (pl, "ld") ||
    IS_RAB && (lineIsInst (pl, "ldp") || lineIsInst (pl, "ldf")) ||
    IS_EZ80 && (lineIsInst (pl, "ld.il") || lineIsInst (pl, "ld.l") || lineIsInst (pl, "ld.lil") || lineIsInst (pl, "ld.lis")))
    {
      if(argCont(strchr(pl->line, ','), what))
        return(true);
      if(*(strchr(pl->line, ',') - 1) == ')' && strstr(pl->line + 3, what) &&
        (strchr(pl->line, '#') == 0 || strchr(pl->line, '#') > strchr(pl->line, ',')) &&
        (strchr(pl->line, '_') == 0 || strchr(pl->line, '_') > strchr(pl->line, ',')))
        return(true);
      if (!strcmp(what, "sp") && strchr(pl->line, '(')) // Assume any indirect memory access to be a possible stack access. This avoids optimizing out stackframe setups for local variables (bug #3173).
        return(true);
      return(false);
    }


  // Sometimes the result and flags do not depend on the value of the operands.
  if (larg &&
    (lineIsInst (pl, "cp") ||
    lineIsInst (pl, "sbc") ||
    lineIsInst (pl, "sbc") ||
    lineIsInst (pl, "xor")))
    {
      if (!strncmp (larg, "a, a", 4) || !strncmp (larg, "hl, hl", 6) || !strncmp (larg, "iy, iy", 6))
        return(false);
    }

  //ld a, #0x00
  if(!strcmp(pl->line, "and\ta, #0x00") || !strcmp(pl->line, "and\ta,#0x00") || !strcmp(pl->line, "and\t#0x00"))
    return(false);

  //ld a, #0xff
  if(!strcmp(pl->line, "or\ta, #0xff") || !strcmp(pl->line, "or\ta,#0xff") || !strcmp(pl->line, "or\t#0xff"))
    return(false);

  if (larg &&
    (lineIsInst (pl, "adc") ||
    lineIsInst (pl, "add") ||
    lineIsInst (pl, "and") ||
    lineIsInst (pl, "or") ||
    lineIsInst (pl, "cp") ||
    lineIsInst (pl, "sbc") ||
    lineIsInst (pl, "sub") ||
    lineIsInst (pl, "xor") ||
    IS_EZ80 && lineIsInst (pl, "add.l") ||
    IS_R800 && lineIsInst (pl, "multu") ||
    IS_R800 && lineIsInst (pl, "multuw")))
    {
      if (!rarg) // Basic support for asm syntax variant that omits left operand on 8-bit oeprations.
        {
          rarg = larg;
          larg = "a";
        }
      if (larg[0] == 'a' && larg[1] == ',')
        {
          if (!strcmp(what, "a"))
            return(true);
        }
      else if (IS_Z80N && !strncmp (larg, "bc", 2) && *(larg + 2) == ',')
        {
          if (!strcmp(what, "b") || !strcmp(what, "c"))
            return(true);
        }
      else if (IS_Z80N && !strncmp (larg, "de", 2) && *(larg + 2) == ',')
        {
          if (!strcmp(what, "d") || !strcmp(what, "e"))
            return(true);
        }
      else if (!strncmp (larg, "hl", 2) && larg[2] == ',') // add hl, rr
        {
          if (!strcmp(what, "h") || !strcmp(what, "l"))
            return(true);
        }
      else if (!strncmp(larg, "sp", 2) && larg[2] == ',') // add sp, rr
        {
          if (!strcmp(what, "sp"))
            return(true);
        }
      else if (larg[0] == 'i') // add ix/y, rr
        {
          if (!strncmp (larg, what, 2))
            return(true);
        }
      return (argCont (rarg, what));
    }

  if(lineIsInst (pl, "neg"))
    return(strcmp(what, "a") == 0);

  if(lineIsInst (pl, "pop") || IS_EZ80 && lineIsInst (pl, "pop.l"))
    return(strcmp(what, "sp") == 0);

  if (larg &&
    (lineIsInst (pl, "push") ||
    IS_EZ80 && lineIsInst (pl, "push.l")))
    return (strstr (larg, what) || !strcmp(what, "sp"));

  if (larg &&
    (lineIsInst (pl, "dec") ||
    lineIsInst (pl, "inc") ||
    IS_EZ80 && lineIsInst (pl, "dec.l") ||
    IS_EZ80 && lineIsInst (pl, "inc.l")))
    {
      return (argCont (larg, what));
    }

  if(lineIsInst (pl, "cpl"))
    return(!strcmp(what, "a"));

  if(lineIsInst (pl, "di") || lineIsInst (pl, "ei"))
    return(false);

  // Rotate and shift group
  if(lineIsInst (pl, "rlca") ||
     lineIsInst (pl, "rla")  ||
     lineIsInst (pl, "rrca") ||
     lineIsInst (pl, "rra")  ||
     lineIsInst (pl, "daa"))
    {
      return(strcmp(what, "a") == 0);
    }
  if (larg &&
    (lineIsInst (pl, "rl") ||
    lineIsInst (pl, "rlc") ||
    lineIsInst (pl, "sla") ||
    lineIsInst (pl, "rr") ||
    lineIsInst (pl, "rrc") ||
    lineIsInst (pl, "sra") ||
    lineIsInst (pl, "srl")))
    {
      return (argCont (larg, what));
    }
  if ((IS_SM83 || IS_Z80N) && larg && lineIsInst (pl, "swap"))
    {
      return (argCont (larg, what));
    }
  if(!IS_SM83 && !IS_RAB &&
    (lineIsInst (pl, "rld") ||
     lineIsInst (pl, "rrd")))
    return(!!strstr("ahl", what));

  // Bit set, reset and test group
  if (lineIsInst (pl, "bit") ||
    lineIsInst (pl, "set") ||
    lineIsInst (pl, "res"))
    {
      return (argCont (rarg, what));
    }

  if(lineIsInst (pl, "ccf") ||
    lineIsInst (pl, "scf")  ||
    lineIsInst (pl, "nop")  ||
    lineIsInst (pl, "halt") ||
    (IS_SM83 && lineIsInst (pl, "stop")))
    return(false);

  if(lineIsInst (pl, "jp") || lineIsInst (pl, "jr"))
    return(false);

  if(lineIsInst (pl, "djnz"))
    return(strchr(what, 'b') != 0);

  if(!IS_SM83 && (lineIsInst (pl, "ldd") || lineIsInst (pl, "lddr") || lineIsInst (pl, "ldi") || lineIsInst (pl, "ldir")))
    return(strchr("bcdehl", *what));
  if(IS_SM83 && (lineIsInst (pl, "ldd") || lineIsInst (pl, "ldi")))
    return(strchr("hl", *what) || strstr(strchr(pl->line + 4, ','), what) != 0);

  if(!IS_SM83 && !IS_RAB && (lineIsInst (pl, "cpd") || lineIsInst (pl, "cpdr") || lineIsInst (pl, "cpi") || lineIsInst (pl, "cpir")))
    return(strchr("abchl", *what));

  if(!IS_SM83 && !IS_RAB && !IS_TLCS90 && lineIsInst (pl, "out"))
    return(strstr(strchr(pl->line + 4, ','), what) != 0 || strstr(pl->line + 4, "(c)") && (!strcmp(what, "b") || !strcmp(what, "c")));
  if(!IS_SM83 && !IS_RAB && !IS_TLCS90 && lineIsInst (pl, "in"))
    return(!strstr(strchr(pl->line + 4, ','), "(c)") && !strcmp(what, "a") || strstr(strchr(pl->line + 4, ','), "(c)") && (!strcmp(what, "b") || !strcmp(what, "c")));

  if(IS_SM83 && (lineIsInst (pl, "out") || lineIsInst (pl, "ldh") || lineIsInst (pl, "in")))
    return(strstr(strchr(pl->line + 3, ','), what) != 0 || (!strcmp(what, "c") && strstr(pl->line + 3, "(c)")));

  if(!IS_SM83 && !IS_RAB &&
    (lineIsInst (pl, "ini") || lineIsInst (pl, "ind") || lineIsInst (pl, "inir") || lineIsInst (pl, "indr") ||
    lineIsInst (pl, "outi") || lineIsInst (pl, "outd") || lineIsInst (pl, "otir") || lineIsInst (pl, "otdr")))
    return(strchr("bchl", *what));

  if((IS_Z180 || IS_EZ80) && (lineIsInst (pl, "in0") || lineIsInst (pl, "out0")))
    return(false);

  if((IS_Z180 || IS_EZ80 || IS_Z80N) && lineIsInst (pl, "mlt"))
    return(argCont(pl->line + 4, what));

  if((IS_Z180 || IS_EZ80) &&
    (lineIsInst (pl, "otim") || lineIsInst (pl, "otimr") || lineIsInst (pl, "otir") || lineIsInst (pl, "otirx")))
    return(strchr("bchl", *what));

  if((IS_Z180 || IS_EZ80) && lineIsInst (pl, "slp"))
    return(false);

  if((IS_Z180 || IS_EZ80 || IS_Z80N) && lineIsInst (pl, "tst"))
    return(argCont(pl->line + 4, what));

  if((IS_Z180 || IS_EZ80) && lineIsInst (pl, "tstio"))
    return(!strcmp(what, "c"));

  if(IS_RAB &&
    (lineIsInst (pl, "ioi") ||
    lineIsInst (pl, "ioe") ||
    lineIsInst (pl, "ipres") ||
    lineIsInst (pl, "ipset0") ||
    lineIsInst (pl, "ipset1") ||
    lineIsInst (pl, "ipset2") ||
    lineIsInst (pl, "ipset3")))
    return(false);

  // Check this here before the rules for the (pre-R6K) mul and mulu variants without arguments below.
  if (IS_R6K && (lineIsInst (pl, "mul") || lineIsInst (pl, "mulu")) && larg && rarg)
    return (argCont (larg, what) || argCont (rarg, what));
 
  if (IS_RAB && lineIsInst (pl, "mul"))
    return (!strcmp(what, "b") || !strcmp(what, "c") || !strcmp(what, "d") || !strcmp(what, "e"));

  if (IS_RAB && larg && lineIsInst (pl, "bool"))
    return (argCont (larg, what));
    
  if((IS_R3KA || IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "lsdr") || lineIsInst (pl, "lidr") || lineIsInst (pl, "lsddr") || lineIsInst (pl, "lsidr"))
    return(strchr("bcdehl", *what));

  if ((IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "clr"))
    return(false);

  if((IS_R4K || IS_R5K || IS_R6K) && (!strcmp(pl->line, "ex\tbc, hl") || !strcmp(pl->line, "ex\tbc,hl")))
    return(!strcmp(what, "h") || !strcmp(what, "l") || !strcmp(what, "b") || !strcmp(what, "c"));
  if((IS_R4K || IS_R5K || IS_R6K) && (!strcmp(pl->line, "ex\tjk, hl") || !strcmp(pl->line, "ex\tjk,hl")))
    return(!strcmp(what, "h") || !strcmp(what, "l") || !strcmp(what, "j") || !strcmp(what, "k"));

  if ((IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "mulu"))
    return (!strcmp(what, "b") || !strcmp(what, "c") || !strcmp(what, "d") || !strcmp(what, "e"));

  if ((IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "test"))
    return (argCont (larg, what));

  if ((IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "cbm"))
    return (strchr("ahlde", *what));

  if (IS_R6K && lineIsInst (pl, "swap"))
    return (argCont (larg, what));

  if(IS_EZ80 && lineIsInst (pl, "lea") ||
    IS_TLCS90 && lineIsInst (pl, "lda"))
    return(argCont(rarg, what) || argCont(lineArg (pl, 2), what));

  if(IS_EZ80 && lineIsInst (pl, "pea"))
    return(argCont(pl->line + 4, what) || !strcmp(what, "sp"));

  if (IS_SM83 && (lineIsInst (pl, "lda") || lineIsInst (pl, "ldhl")))
    return(!strcmp(what, "sp"));

  if(IS_Z80N &&
    (lineIsInst (pl, "bsla") ||
    lineIsInst (pl, "bsra") ||
    lineIsInst (pl, "bsrl") ||
    lineIsInst (pl, "bsrf") ||
    lineIsInst (pl, "brlc")))
    return(strchr("bde", *what));

  if(IS_TLCS90 &&
    (lineIsInst (pl, "decx") ||
    lineIsInst (pl, "incx")))
    return(false);
  if (IS_TLCS90 &&
    (lineIsInst (pl, "mul") ||
    lineIsInst (pl, "div")))
    {
      if(!strcmp(what, "h") || !strcmp(what, "l"))
        return true;
      wassert (rarg);
      if (rarg[0] == what[0])
        return true;
      if (strchr (rarg, '('));
        return (!strcmp(what, "ix") || !strcmp(what, "iy") || !strcmp(what, "hl") || !strcmp(what, "sp"));
    }

  /* TODO: Can we know anything about rst? */
  if(lineIsInst (pl, "rst"))
    return(true);

  printf("z80MightRead unknown asm inst line: %s\n", pl->line);

  return(true);
}

static bool
z80UncondJump(const lineNode *pl)
{
  if((lineIsInst (pl, "jp") || lineIsInst (pl, "jr")) &&
     strchr(pl->line, ',') == 0)
    return TRUE;
  return FALSE;
}

static bool
z80CondJump(const lineNode *pl)
{
  if(((lineIsInst (pl, "jp") || lineIsInst (pl, "jr")) &&
      strchr(pl->line, ',') != 0) ||
     lineIsInst (pl, "djnz"))
    return TRUE;
  return FALSE;
}

// TODO: z80 flags only partly implemented
static bool
z80SurelyWritesFlag(const lineNode *pl, const char *what)
{
  /* LD instruction is never change flags except LD A,I and LD A,R.
    But it is most popular instruction so place it first */
  if(lineIsInst (pl, "ld"))
    {
      if(IS_SM83 || IS_RAB || !!strcmp(what, "pf") ||
          !argCont(pl->line+3, "a"))
        return false;
      const char *p = strchr(pl->line+4, ',');
      if (p == NULL)
        return false; /* unknown instruction */
      ++p;
      return argCont(p, "i") || argCont(p, "r");
    }

  if(!IS_SM83 && !IS_RAB && !IS_TLCS90 && lineIsInst (pl, "in"))
    {
      if(strstr(strchr(pl->line + 4, ','), "(c)") || strstr(strchr(pl->line + 4, ','), "(bc)"))
        return !!strcmp(what, "cf");
      else
        return false;
    }

  if(lineIsInst (pl, "rlca") ||
     lineIsInst (pl, "rrca") ||
     lineIsInst (pl, "rra")  ||
     lineIsInst (pl, "rla"))
    return(IS_SM83 || !!strcmp(what, "zf") && !!strcmp(what, "sf") && !!strcmp(what, "pf"));

  if(lineIsInst (pl, "adc") ||
     lineIsInst (pl, "and") ||
     lineIsInst (pl, "sbc") ||
     lineIsInst (pl, "sub") ||
     lineIsInst (pl, "xor") ||
     lineIsInst (pl, "and") ||
     lineIsInst (pl, "rlc") ||
     lineIsInst (pl, "rrc") ||
     lineIsInst (pl, "sla") ||
     lineIsInst (pl, "sra") ||
     lineIsInst (pl, "srl") ||
     lineIsInst (pl, "neg"))
    return true;

  if(lineIsInst (pl, "or") ||
     lineIsInst (pl, "cp") ||
     lineIsInst (pl, "rl") ||
     lineIsInst (pl, "rr"))
    return true;

  if((IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "test"))
    return true;

  if(lineIsInst (pl, "bit") ||
     lineIsInst (pl, "cpd") ||
     lineIsInst (pl, "cpi") ||
     lineIsInst (pl, "ind") ||
     lineIsInst (pl, "ini") ||
     lineIsInst (pl, "rrd"))
    return (!!strcmp(what, "cf"));

  if(lineIsInst (pl, "cpdr") ||
     lineIsInst (pl, "cpir") ||
     lineIsInst (pl, "indr") ||
     lineIsInst (pl, "inir") ||
     lineIsInst (pl, "otdr") ||
     lineIsInst (pl, "otir") ||
     lineIsInst (pl, "outd") ||
     lineIsInst (pl, "outi"))
    return (!!strcmp(what, "cf"));

  if(lineIsInst (pl, "daa"))
    return (!!strcmp(what, "nf"));

  if(lineIsInst (pl, "ccf") ||
    lineIsInst (pl, "scf"))
    return (!strcmp(what, "hf") || !strcmp(what, "nf") || !strcmp(what, "cf"));

  if(lineIsInst (pl, "cpl"))
    return (!strcmp(what, "hf") || !strcmp(what, "nf"));

  if(lineIsInst (pl, "inc") || lineIsInst (pl, "dec") ||
    IS_EZ80 && lineIsInst (pl, "inc.l") || IS_EZ80 && lineIsInst (pl, "dec.l"))
    {
      // 8-bit inc affects all flags other than c.
      if (strlen(pl->line + 4) == 1 || // 8-bit register
        !strcmp(pl->line + 4, "(hl)") ||
        !strcmp(pl->line + 6, "(ix)") ||
        !strcmp(pl->line + 6, "(iy)"))
        return (!!strcmp(what, "cf"));
      return false; // 16-bit inc does not affect flags.
    }

  if(lineIsInst (pl, "add"))
    return (argCont(pl->line + 4, "a") ||
           (!!strcmp(what, "zf") && !!strcmp(what, "sf") && !!strcmp(what, "pf")));

  if(lineIsInst (pl, "ldd") ||
    lineIsInst (pl, "lddr") ||
    lineIsInst (pl, "ldi") ||
    lineIsInst (pl, "ldir"))
    return (!strcmp(what, "hf") || !strcmp(what, "pf") || !strcmp(what, "nf"));

  // pop af writes
  if(lineIsInst (pl, "pop"))
    return (argCont(pl->line + 4, "af"));

  // according to calling convention caller has to save flags
  if(lineIsInst (pl, "ret") ||
     lineIsInst (pl, "call"))
    return true;

  if(lineIsInst (pl, "rld") ||
    lineIsInst (pl, "rrd"))
    return (!strcmp(what, "hf") || !strcmp(what, "pf") || !strcmp(what, "nf"));

  if(lineIsInst (pl, "di") ||
    lineIsInst (pl, "djnz") ||
    lineIsInst (pl, "ei") ||
    lineIsInst (pl, "ex") ||
    lineIsInst (pl, "nop") ||
    lineIsInst (pl, "out") ||
    lineIsInst (pl, "push") ||
    lineIsInst (pl, "res") ||
    lineIsInst (pl, "set"))
    return false;

  if (IS_Z80N &&
    (lineIsInst (pl, "bsla") ||
    lineIsInst (pl, "bsra") ||
    lineIsInst (pl, "bsrl") ||
    lineIsInst (pl, "bsrf") ||
    lineIsInst (pl, "swap")))
    return false;
    
  if(IS_SM83 && lineIsInst (pl, "ldh"))
    return false;
    
  if(IS_SM83 &&
     (lineIsInst (pl, "swap") ||
      lineIsInst (pl, "ldhl") ||
      lineIsInst (pl, "lda")))
    return true;

  /* handle IN0 r,(n) and IN r,(c) instructions */
  if(lineIsInst (pl, "in0") || (lineIsInst (pl, "in") && (!strcmp(pl->line+5, "(c)") || !strcmp(pl->line+5, "(bc)"))))
    return (!!strcmp(what, "cf"));

  if((IS_Z180 || IS_EZ80) && lineIsInst (pl, "out0"))
    return false;

  if(IS_RAB &&
    lineIsInst (pl, "bool"))
    return true;

  if(IS_RAB &&
    (lineIsInst (pl, "ldp") ||
    lineIsInst (pl, "ioi") ||
    lineIsInst (pl, "ipres") ||
    lineIsInst (pl, "mul")))
    return false;

  if((IS_R3KA || IS_R4K || IS_R5K || IS_R6K) &&
     (lineIsInst (pl, "lsddr") ||
     lineIsInst (pl, "lsidr")))
    return false;

  if((IS_R4K || IS_R5K || IS_R6K) &&
    (lineIsInst (pl, "cbm") ||
    lineIsInst (pl, "clr") ||
    lineIsInst (pl, "ldf") ||
    lineIsInst (pl, "mulu")))
    return false;

  if (IS_R6K && lineIsInst (pl, "swap"))
    return false;

  if(lineIsInst (pl, "mlt"))
    return (!IS_Z80N);

  if((IS_Z180 || IS_EZ80 || IS_Z80N) &&
    lineIsInst (pl, "tst"))
    return true;

  if(IS_EZ80 && lineIsInst (pl, "add.l"))
    return (!!strcmp(what, "zf") && !!strcmp(what, "sf") && !!strcmp(what, "pf"));

  if(IS_EZ80 &&
     (lineIsInst (pl, "adc") ||
     lineIsInst (pl, "sbc")))
    return true;

  if(IS_EZ80 &&
    (lineIsInst (pl, "ld.il") ||
    lineIsInst (pl, "ld.is") ||
    lineIsInst (pl, "ld.l") ||
    lineIsInst (pl, "ld.lil") ||
    lineIsInst (pl, "ld.lis") ||
    lineIsInst (pl, "lea") ||
    lineIsInst (pl, "pea") ||
    lineIsInst (pl, "push.l") ||
    lineIsInst (pl, "res.l") ||
    lineIsInst (pl, "set.l")))
    return false;

  // pop af writes
  if(lineIsInst (pl, "pop.l"))
    return (argCont(pl->line + 6, "af"));

  if(IS_TLCS90 &&
    (lineIsInst (pl, "lda") ||
    lineIsInst (pl, "mul")))
    return false;

  if(IS_TLCS90 &&
    (lineIsInst (pl, "decx") ||
    lineIsInst (pl, "incx")))
    return (!!strcmp(what, "cf"));

  if(IS_R800 && lineIsInst (pl, "multu") ||
     IS_R800 && lineIsInst (pl, "multuw"))
     return (strcmp(what, "hf") && strcmp(what, "sf"));

  printf("Warning: z80SurelyWritesFlag unknown asm inst line: %s\n", pl->line);

  return false; // Fail-safe: we have no idea what happens at this line, so assume it writes nothing.
}

static bool
callSurelyWrites (const lineNode *pl, const char *what)
{
  const symbol *f = 0;
  if (lineIsInst (pl, "call") && !strchr(pl->line, ','))
    f = findSym (SymbolTab, 0, pl->line + 6);
  else if ((lineIsInst (pl, "jp") || lineIsInst (pl, "jr")) && !strchr(pl->line, ','))
    f = findSym (SymbolTab, 0, pl->line + 4);

  const bool *preserved_regs;

  if (f && (strlen(what) == 2 && what[1] == 'f')) // Flags are never preserved across function calls.
    return(true);

  if(!strcmp(what, "ix"))
    return(false);

  if(f)
    preserved_regs = f->type->funcAttrs.preserved_regs;
  else if (lineIsInst (pl, "call"))
    preserved_regs = z80_regs_preserved_in_calls_from_current_function;
  else // Err on the safe side for jp and jr - might not be a function call, might e.g. be a jump table.
    return (false);

  if (!strcmp (what, "a"))
    return !preserved_regs[A_IDX];
  if (!strcmp (what, "c"))
    return !preserved_regs[C_IDX];
  if (!strcmp (what, "b"))
    return !preserved_regs[B_IDX];
  if (!strcmp (what, "e"))
    return !preserved_regs[E_IDX];
  if (!strcmp (what, "d"))
    return !preserved_regs[D_IDX];
  if (!strcmp (what, "l"))
    return !preserved_regs[L_IDX];
  if (!strcmp (what, "h"))
    return !preserved_regs[H_IDX];
  if (!strcmp (what, "iyl"))
    return !preserved_regs[IYL_IDX];
  if (!strcmp (what, "iyh"))
    return !preserved_regs[IYH_IDX];
  if (!strcmp (what, "iy"))
    return !preserved_regs[IYL_IDX] && !preserved_regs[IYH_IDX];

  return (false);
}

static bool
z80SurelyWrites (const lineNode *pl, const char *what)
{
  if(strcmp(what, "iyl") == 0 || strcmp(what, "iyh") == 0)
    what = "iy";
  if(strcmp(what, "ixl") == 0 || strcmp(what, "ixh") == 0)
    what = "ix";

  const char *larg = lineArg (pl, 0);
  const char *rarg = lineArg (pl, 1);

  //ld a, #0x00
  if((lineIsInst (pl, "xor") || lineIsInst (pl, "sub")) && !strcmp(what, "a") &&
     (!strcmp(pl->line+4, "a, a") || !strcmp(pl->line+4, "a,a") || (!strchr(pl->line, ',') && !strcmp(pl->line+4, "a"))))
    return(true);

  //ld a, #0x00
  if(!strcmp(what, "a") && (!strcmp(pl->line, "and\ta, #0x00") || !strcmp(pl->line, "and\ta,#0x00") || !strcmp(pl->line, "and\t#0x00")))
    return(true);

  //ld a, #0xff
  if(!strcmp(what, "a") && (!strcmp(pl->line, "or\ta, #0xff") || !strcmp(pl->line, "or\ta,#0xff") || !strcmp(pl->line, "or\t#0xff")))
    return(true);

  if (lineIsInst (pl, "adc") ||
    lineIsInst (pl, "add") ||
    lineIsInst (pl, "and") ||
    lineIsInst (pl, "dec") ||
    lineIsInst (pl, "inc") ||
    lineIsInst (pl, "or") ||
    lineIsInst (pl, "sbc") ||
    lineIsInst (pl, "sra") ||
    lineIsInst (pl, "srl") ||
    lineIsInst (pl, "sub") ||
    lineIsInst (pl, "xor"))
    {
      if (!strcmp (what, "a") && larg && larg[0] == 'a')
        return(true);
      if ((!strcmp (what, "h") || !strcmp (what, "l")) && larg && !strncmp (larg, "hl", 2))
        return(true);
      return(false);
    }

  if (lineIsInst (pl, "ccf") ||
    lineIsInst (pl, "ei") ||
    lineIsInst (pl, "di") ||
    lineIsInst (pl, "scf"))
    return (false);

  if (lineIsInst (pl, "cp"))
    return (false);

  if (lineIsInst (pl, "cpl") ||
    lineIsInst (pl, "daa") ||
    lineIsInst (pl, "rla") ||
    lineIsInst (pl, "rra") ||
    lineIsInst (pl, "rlca") ||
    lineIsInst (pl, "rrca"))
    return (what[0] == 'a');

  if (larg && lineIsInst (pl, "ex"))
    return (strstr (larg, what) || strstr (rarg, what));

  if (larg && lineIsInst (pl, "ld") && (larg[0] == '-' || larg[0] == '(' || isdigit (larg[0])))
    return (false);
  if (larg && lineIsInst (pl, "ld") && !strncmp (larg, "hl,", 3))
    return(what[0] == 'h' || what[0] == 'l');
  if (larg && lineIsInst (pl, "ld") && !strncmp (larg, "de,", 3))
    return(what[0] == 'd' || what[0] == 'e');
  if (larg && lineIsInst (pl, "ld") && !strncmp (larg, "bc,", 3))
    return(what[0] == 'b' || what[0] == 'c');
  if (larg && (IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "ld") && !strncmp (larg, "bcde,", 5))
    return(strchr ("bcde", *what));
  if (larg && (IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "ld") && !strncmp (larg, "jkhl,", 5))
    return(strchr ("jkhl", *what));
  if (larg && lineIsInst (pl, "ld") && !strncmp (larg, "ix,", 3))
    return(!strcmp (what, "ix"));
  if (larg && lineIsInst (pl, "ld") && !strncmp (larg, "iy,", 3))
    return(!strcmp (what, "iy"));
  if (larg && (lineIsInst (pl, "ld") || lineIsInst (pl, "in"))
    && strlen(what) > 1 && larg && larg[0] && larg[1] == ',')
    return (false);
  if (larg && ((lineIsInst (pl, "ld") || lineIsInst (pl, "in")))
    && strlen (larg) >= strlen (what) && larg[strlen (what)] == ',')
    return (!strncmp (larg, what, strlen (what)));

  if (!IS_SM83 && (lineIsInst (pl, "ldir") || lineIsInst (pl, "lddr")))
    return(strchr ("bcdehl", *what));

  if(IS_RAB && lineIsInst (pl, "ldp") && strncmp(pl->line + 4, "hl", 2) == 0 && (what[0] == 'h' || what[0] == 'l'))
    return(true);

  if(IS_SM83 && (lineIsInst (pl, "ldd") || lineIsInst (pl, "ldi") || lineIsInst (pl, "ldh")))
    return(strncmp(pl->line + 4, what, strlen(what)) == 0);

  if (larg && lineIsInst (pl, "pop") && !strncmp (larg, "af", 2))
    return (what[0] == 'a');
  else if (larg && lineIsInst (pl, "pop") && !strncmp (larg, "ix", 2))
    return (!strcmp (what, "ix"));
  else if (larg && lineIsInst (pl, "pop") && !strncmp (larg, "iy", 2))
    return (!strcmp (what, "iy"));
  else if (larg && lineIsInst (pl, "pop"))
    return (strstr (larg, what));
  
  if (lineIsInst (pl, "call") && strchr(pl->line, ',') == 0)
    return (callSurelyWrites (pl, what));

  if(strcmp(pl->line, "ret") == 0)
    return true;

  if(lineIsInst (pl, "bit") ||
    lineIsInst (pl, "push"))
    return (false);

  if (IS_Z180 || IS_EZ80 || IS_Z80N)
    if (larg && lineIsInst (pl, "mlt"))
      return (strchr (larg, *what));

  if (IS_Z180 || IS_EZ80)
    {
      if (lineIsInst (pl, "otim") ||
        lineIsInst (pl, "otimr") ||
        lineIsInst (pl, "otdm") ||
        lineIsInst (pl, "otdmr"))
        return (strchr("bchl", *what));

      if (larg && lineIsInst (pl, "in0"))
        return (!strncmp (larg, what, strlen (what)));
      if (lineIsInst (pl, "out0"))
        return (false);
    }

  if (IS_EZ80 && larg && lineIsInst (pl, "lea"))
    return (strstr (larg, what));

  if (IS_SM83 && lineIsInst (pl, "lda") && strncmp(pl->line + 4, "hl", 2) == 0 && (what[0] == 'h' || what[0] == 'l'))
    return (true);

  if (IS_SM83 && lineIsInst (pl, "ldhl") && (what[0] == 'h' || what[0] == 'l'))
    return (true);

  if (IS_RAB && larg && lineIsInst (pl, "bool"))
    return (!strncmp (larg, what, strlen(what)) || strchr (larg, *what));

  if (IS_RAB && lineIsInst (pl, "cbm"))
    return (false);

  // Check this here before the rules for the (pre-R6K) mul and mulu variants without arguments below.
  if (IS_R6K && (lineIsInst (pl, "mul") || lineIsInst (pl, "mulu")) && larg && rarg &&
    !STRNCASECMP (larg, "hl", 2) && !STRNCASECMP (rarg, "de", 2))
    return (strchr ("jkhl", *what));

  if (IS_RAB && lineIsInst (pl, "mul"))
    return (strchr ("hlbc", *what));
  if ((IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "mulu"))
    return (strchr ("hlbc", *what));

  if ((IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "test"))
    return (false);

  if (IS_R800 && lineIsInst (pl, "multu"))
    return (strchr ("hl", *what));

  if (IS_R800 && lineIsInst (pl, "multuw"))
    return (strchr ("dehl", *what));

  //printf("Warning: z80SurelyWrites unknown asm inst line: %s\n", pl->line);
  //printf("larg '%s' what '%s'\n", larg ? larg : "", what);

  return(false);
}

static bool
z80SurelyReturns(const lineNode *pl)
{
  if(strcmp(pl->line, "ret") == 0)
    return TRUE;
  return FALSE;
}

/*-----------------------------------------------------------------*/
/* scan4op - "executes" and examines the assembler opcodes,        */
/* follows conditional and un-conditional jumps.                   */
/* Moreover it registers all passed labels.                        */
/*                                                                 */
/* Parameter:                                                      */
/*    lineNode **pl                                                */
/*       scanning starts from pl;                                  */
/*       pl also returns the last scanned line                     */
/*    const char *pReg                                             */
/*       points to a register (e.g. "ar0"). scan4op() tests for    */
/*       read or write operations with this register               */
/*    const char *untilOp                                          */
/*       points to NULL or a opcode (e.g. "push").                 */
/*       scan4op() returns if it hits this opcode.                 */
/*    lineNode **plCond                                            */
/*       If a conditional branch is met plCond points to the       */
/*       lineNode of the conditional branch                        */
/*                                                                 */
/* Returns:                                                        */
/*    S4O_ABORT                                                    */
/*       on error                                                  */
/*    S4O_VISITED                                                  */
/*       hit lineNode with "visited" flag set: scan4op() already   */
/*       scanned this opcode.                                      */
/*    S4O_FOUNDOPCODE                                              */
/*       found opcode and operand, to which untilOp and pReg are   */
/*       pointing to.                                              */
/*    S4O_RD_OP, S4O_WR_OP                                         */
/*       hit an opcode reading or writing from pReg                */
/*    S4O_CONDJMP                                                  */
/*       hit a conditional jump opcode. pl and plCond return the   */
/*       two possible branches.                                    */
/*    S4O_TERM                                                     */
/*       acall, lcall, ret and reti "terminate" a scan.            */
/*-----------------------------------------------------------------*/
static S4O_RET
scan4op (lineNode **pl, const char *what, const char *untilOp,
         lineNode **plCond)
{
  bool isFlag = (strlen(what) == 2 && what[1] == 'f');
  while (*pl)
    {
      if (!(*pl)->line || (*pl)->isDebug || (*pl)->isComment || (*pl)->isLabel)
        {
          *pl = (*pl)->next;
          continue;
        }
      D(("Scanning %s for %s\n", (*pl)->line, what));
      /* don't optimize across inline assembler,
         e.g. isLabel doesn't work there */
      if ((*pl)->isInline)
        {
          D(("S4O_RD_OP: Inline asm\n"));
          return S4O_ABORT;
        }

      if ((*pl)->visited)
        {
          D(("S4O_VISITED\n"));
          return S4O_VISITED;
        }

      (*pl)->visited = TRUE;

      if(isFlag ? z80MightReadFlag(*pl, what) : z80MightRead (*pl, what))
        {
          D(("S4O_RD_OP (flag)\n"));
          return S4O_RD_OP;
        }

      if (z80UncondJump (*pl))
        {
          lineNode *tlbl = findLabel (*pl);
          if (!tlbl) // jp/jr could be a tail call.
            {
              const symbol *f = findSym (SymbolTab, 0, (*pl)->line + 4);
              if (f && z80IsParmInCall(f->type, what))
                {
                  D (("S4O_RD_OP\n"));
                  return S4O_RD_OP;
                }
              else if(callSurelyWrites (*pl, what))
                {
                  D (("S4O_WR_OP\n"));
                  return S4O_WR_OP;
                }
            }
          *pl = tlbl;
          continue;
        }
      if (z80CondJump(*pl))
        {
          *plCond = findLabel (*pl);
          if (!*plCond)
            {
              D (("S4O_ABORT\n"));
              return S4O_ABORT;
            }
          D (("S4O_CONDJMP\n"));
          return S4O_CONDJMP;
        }

      if (isFlag ? z80SurelyWritesFlag (*pl, what) : z80SurelyWrites(*pl, what))
        {
          D (("S4O_WR_OP (flag)\n"));
          return S4O_WR_OP;
        }

      /* Don't need to check for de, hl since z80MightRead() does that */
      if(z80SurelyReturns(*pl))
        {
          D(("S4O_TERM\n"));
          return S4O_TERM;
        }

       *pl = (*pl)->next;
    }
  D(("S4O_ABORT\n"));
  return S4O_ABORT;
}

/*-----------------------------------------------------------------*/
/* doTermScan - scan through area 2. This small wrapper handles:   */
/* - action required on different return values                    */
/* - recursion in case of conditional branches                     */
/*-----------------------------------------------------------------*/
static bool
doTermScan (lineNode **pl, const char *what)
{
  lineNode *plConditional;

  for (;; *pl = (*pl)->next)
    {
      switch (scan4op (pl, what, NULL, &plConditional))
        {
          case S4O_TERM:
          case S4O_VISITED:
          case S4O_WR_OP:
            /* all these are terminating conditions */
            return TRUE;
          case S4O_CONDJMP:
            /* two possible destinations: recurse */
              {
                lineNode *pl2 = plConditional;
                D(("CONDJMP trying other branch first\n"));
                if (!doTermScan (&pl2, what))
                  return FALSE;
                D(("Other branch OK.\n"));
              }
            continue;
          case S4O_RD_OP:
          default:
            /* no go */
            return FALSE;
        }
    }
}

/* Regular 8 bit reg */
static bool
isReg(const char *what)
{
  if(strlen(what) != 1)
    return FALSE;
  switch(*what)
    {
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'h':
    case 'l':
      return TRUE;
    }
  return FALSE;
}

/* 8-Bit reg only accessible by 16-bit and undocumented instructions */
static bool
isUReg(const char *what)
{
  if(strcmp(what, "iyl") == 0 || strcmp(what, "iyh") == 0)
    return TRUE;
  if(strcmp(what, "ixl") == 0 || strcmp(what, "ixh") == 0)
    return TRUE;
  return FALSE;
}

static bool
isRegPair(const char *what)
{
  if(strlen(what) != 2)
    return FALSE;
  if(strcmp(what, "bc") == 0)
    return TRUE;
  if(strcmp(what, "de") == 0)
    return TRUE;
  if(strcmp(what, "hl") == 0)
    return TRUE;
  if(strcmp(what, "ix") == 0)
    return TRUE;
  if(strcmp(what, "iy") == 0)
    return TRUE;
  return FALSE;
}

/* Check that what is never read after endPl. */

bool
z80notUsed (const char *what, lineNode *endPl, lineNode *head)
{
  lineNode *pl;
  D(("Checking for %s\n", what));

  if(strcmp(what, "af") == 0)
    {
      if(!z80notUsed("a", endPl, head))
        return FALSE;
      what++;
    }

  if(strcmp(what, "f") == 0)
    return z80notUsed("zf", endPl, head) && z80notUsed("cf", endPl, head) &&
           z80notUsed("sf", endPl, head) && z80notUsed("pf", endPl, head) &&
           z80notUsed("nf", endPl, head) && z80notUsed("hf", endPl, head);

  if(strcmp(what, "iy") == 0)
    {
      if(IY_RESERVED)
        return FALSE;
      return(z80notUsed("iyl", endPl, head) && z80notUsed("iyh", endPl, head));
    }

  if(strcmp(what, "ix") == 0)
    return(z80notUsed("ixl", endPl, head) && z80notUsed("ixh", endPl, head));

  if(isRegPair(what))
    {
      char low[2], high[2];
      low[0] = what[1];
      high[0] = what[0];
      low[1] = 0;
      high[1] = 0;
      return(z80notUsed(low, endPl, head) && z80notUsed(high, endPl, head));
    }

  // P/V and L/V (rabbits) are the same flag
  if(!strcmp(what, "vf") || !strcmp(what, "lf"))
    what = "pf";

  // SM83 does not use what it does not have
  // but this allows to write rules for all Z80ies
  if(IS_SM83 && (!strcmp(what, "sf") || !strcmp(what, "pf")))
    {
      D(("Flag %s does not exist\n", what));
      return true;
    }

  // enable sp and flags
  if(!isReg(what) && !isUReg(what) &&
     strcmp(what, "sp") && strcmp(what+1, "f"))
    return FALSE;

  _G.head = head;

  unvisitLines (_G.head);

  pl = endPl->next;
  if (!doTermScan (&pl, what))
    return FALSE;

  return TRUE;
}

bool
z80notUsedFrom (const char *what, const char *label, lineNode *head)
{
  lineNode *cpl;

  for (cpl = head; cpl; cpl = cpl->next)
    {
      if (cpl->isLabel && !strncmp (label, cpl->line, strlen(label)))
        {
          return z80notUsed (what, cpl, head);
        }
    }

  return false;
}

bool
z80canAssign (const char *op1, const char *op2, const char *exotic)
{
  const char *dst, *src;

  // Indexed accesses: One is the indexed one, the other one needs to be a reg or immediate.
  if(exotic)
  {
    if(!strcmp(exotic, "ix") || !strcmp(exotic, "iy"))
    {
      if(isReg(op1) || (IS_EZ80 && isRegPair(op1)))
        return TRUE;
    }
    else if(!strcmp(op2, "ix") || !strcmp(op2, "iy"))
    {
      if(isReg(exotic) || exotic[0] == '#' || (IS_EZ80 && isRegPair(exotic)))
        return TRUE;
    }

    return FALSE;
  }

  // Everything else.
  dst = op1;
  src = op2;

  // 8-bit regs can be assigned to each other directly.
  if(isReg(dst) && isReg(src))
    return true;
  if(HAS_IYL_INST) // With some restrictions also for iyl, iyh, ixl, ixh.
    {
      if (isUReg(dst) && isReg(src) && src[0] <= 'e')
        return true;
      if (isUReg(src) && isReg(dst) && dst[0] <= 'e')
        return true;    
      if (isUReg(dst) && isUReg(src) && src[1] == dst[1])
        return true;
    }

  // Immediates van be loaded into 8-bit registers.
  if((isReg(dst) || HAS_IYL_INST && isUReg(dst)) && src[0] == '#')
    return true;

  // Same if at most one of them is (hl).
  if((isReg(dst) || (IS_EZ80 && isRegPair(dst))) && !strcmp(src, "(hl)"))
    return TRUE;
  if(!strcmp(dst, "(hl)") && (isReg(src) || (IS_EZ80 && isRegPair(src))))
    return TRUE;

  // Can assign between a and (bc), (de), (hl+), (hl-)
  if(!strcmp(dst, "a") &&
     (!strcmp(src, "(bc)") || !strcmp(src, "(de)") || !strcmp(src, "(hl+)") || !strcmp(src, "(hl-)")))
    return TRUE;
  if((!strcmp(dst, "(bc)") || !strcmp(dst, "(de)") || !strcmp(src, "(hl+)") || !strcmp(src, "(hl-)"))
     && !strcmp(src, "a"))
    return TRUE;

  // Can load immediate values directly into registers and register pairs.
  if((isReg(dst) || isRegPair(dst) || !strcmp(src, "sp")) && src[0] == '#')
    return TRUE;

  if((!strcmp(dst, "a") || (!IS_SM83 && (isRegPair(dst) || !strcmp(src, "sp")))) && !strncmp(src, "(#", 2))
    return TRUE;
  if(!strncmp(dst, "(#", 2) && (!strcmp(src, "a") || (!IS_SM83 && isRegPair(src)) || !strcmp(src, "sp")))
    return TRUE;

  // Can load immediate values directly into (hl).
  if(!strcmp(dst, "(hl)") && src[0] == '#')
    return true;

  // Can load between hl / ix / iy and sp.
  if(!strcmp(dst, "sp") && (!strcmp(src, "hl") || !strcmp(src, "ix") || !strcmp(src, "iy")) ||
    (!strcmp(dst, "hl") || !strcmp(dst, "ix") || !strcmp(dst, "iy")) && !strcmp(src, "sp"))
    return true;

  // Rabbit can load between iy and hl.
  if (IS_RAB &&
    (!strcmp(dst, "hl") && (!strcmp(src, "ix") || !strcmp(src, "iy")) ||
    (!strcmp(dst, "ix") || !strcmp(dst, "iy")) && !strcmp(src, "hl")))
    return true;

  return false;
}

static const char *
registerBaseName (const char *op)
{
  if (!strcmp (op, "d") || !strcmp (op, "e") || !strcmp (op, "(de)"))
    return "de";
  if (!strcmp (op, "b") || !strcmp (op, "c") || !strcmp (op, "(bc)"))
    return "bc";
  if (!strcmp (op, "h") || !strcmp (op, "l") || !strcmp (op, "(hl)") || !strcmp (op, "(hl+)")  || !strcmp (op, "(hl-)"))
    return "hl";
  if (!strcmp (op, "iyh") || !strcmp (op, "iyl") || strstr (op, "iy"))
    return "iy";
  if (!strcmp (op, "ixh") || !strcmp (op, "ixl") || strstr (op, "ix"))
    return "ix";
  if (!strcmp (op, "a"))
    return "af";
  return op;
}

// canJoinRegs(reg_hi reg_lo [dst]) returns TRUE,
bool z80canJoinRegs (const char **regs, char dst[20])
{
  //check for only 2 source registers
  if (!regs[0] || !regs[1] || regs[2])
    return FALSE;
  size_t l1 = strlen (regs[0]);
  size_t l2 = strlen (regs[1]);
  if (l1 + l2 >= 20)
    return FALSE;
  if (l1 == 0 || l2 == 0)
    {
      if (l1 == 0 && l2 == 0)
        return FALSE;
      strcpy (dst, registerBaseName (regs[l1 ? 0 : 1]));
    }
  else
    {
      memcpy (&dst[0], regs[0], l1);
      memcpy (&dst[l1], regs[1], l2 + 1); //copy including \0
    }
  if (!strcmp (dst, "ixhixl") || !strcmp (dst, "iyhiyl"))
    {
      if (IS_SM83)
        return FALSE;
      dst[2] = '\0';
    }
  return isRegPair (dst);
}

bool z80canSplitReg (const char *reg, char dst[][16], int nDst)
{
  int i;
  if (nDst < 0 || nDst > 2)
    return FALSE;
  if (!strcmp (reg, "bc") || !strcmp (reg, "de") || !strcmp (reg, "hl"))
    {
      for (i = 0; i < nDst; ++i)
        {
          dst[i][0] = reg[i];
          dst[i][1] = '\0';
        }
    }
  else if (HAS_IYL_INST && (!strcmp (reg, "ix") || !strcmp (reg, "iy")))
    {
      for (i = 0; i < nDst; ++i)
        {
          dst[i][0] = reg[0];
          dst[i][1] = reg[1];
          dst[i][2] = "hl"[i];
          dst[i][3] = '\0';
        }
    }
  else
    return FALSE;

  return TRUE;
}

int z80instructionSize (lineNode *pl)
{
  const char *op0start = lineArg (pl, 0);
  const char *op1start = lineArg (pl, 1);

  if (TARGET_IS_TLCS90) // Todo: More accurate estimate.
    return(6);

  /* All ld instructions */
  if(lineIsInst (pl, "ld"))
    {
      // Rabbit 4000 32-bit loads
      if ((IS_R4K || IS_R5K || IS_R6K) && (!STRNCASECMP (op0start, "bcde", 4) || !STRNCASECMP (op0start, "jkhl", 4)))
        {
          if (op1start[0] == '(' && !STRNCASECMP(op1start, "(hl)", 2))
            return (4); // ld bcde, (mn)
          else if (argCont (op1start, "(ix)") || argCont (op1start, "(iy)") || argCont (op1start, "(sp)"))
            return (3); // ld bcde, d(ix)/n(sp)
          else
            return(2);
        }
      if ((IS_R4K || IS_R5K || IS_R6K) && (!STRNCASECMP (op1start, "bcde", 4) || !STRNCASECMP (op1start, "jkhl", 4)))
        {
          if (op1start[0] == '(' && !STRNCASECMP(op0start, "(hl)", 2))
            return (4); // ld (mn), bcde
          else if (argCont (op0start, "(ix)") || argCont (op0start, "(iy)") || argCont (op0start, "(sp)"))
            return (3); // ld d(ix)/n(sp), bcde
          else
            return(2);
        }

      // Rabbit 16-bit pointer load
      if(IS_RAB && !STRNCASECMP(op0start, "hl", 2) && (argCont (op1start, "(hl)") || argCont (op1start, "(iy)")))
        return(3);
      if(IS_RAB && !STRNCASECMP(op0start, "hl", 2) && (argCont (op1start, "(sp)") || argCont (op1start, "(ix)")))
        return(2);
      if(IS_RAB && (argCont (op0start, "(hl)") || argCont (op0start, "(iy)")) && !STRNCASECMP(op1start, "hl", 2) )
        return(3);
      if(IS_RAB && (argCont (op0start, "(sp)") || argCont (op0start, "(ix)")) && !STRNCASECMP(op1start, "hl", 2))
        return(2);

      // These 4 are the only cases of 4 byte long Z80 ld instructions.
      if(!STRNCASECMP (op0start, "ix", 2) || !STRNCASECMP (op0start, "iy", 2))
        return(4);
      if((argCont (op0start, "(ix)") || argCont (op0start, "(iy)")) && op1start[0] == '#')
        return(4);

      if(op0start[0] == '('               && STRNCASECMP(op0start, "(bc)", 4) &&
         STRNCASECMP(op0start, "(de)", 4) && STRNCASECMP(op0start, "(hl" , 3) &&
         STRNCASECMP(op1start, "hl", 2)   && STRNCASECMP(op1start, "a", 1)   &&
         (!IS_SM83 || STRNCASECMP(op1start, "sp", 2)) ||
         op1start[0] == '('               && STRNCASECMP(op1start, "(bc)", 4) &&
         STRNCASECMP(op0start, "(de)", 4) && STRNCASECMP(op1start, "(hl" , 3) &&
         STRNCASECMP(op0start, "hl", 2)   && STRNCASECMP(op0start, "a", 1))
        return(4);

      /* eZ80 16-bit pointer load */
      if(IS_EZ80 &&
        (!STRNCASECMP(op0start, "bc", 2) || !STRNCASECMP(op0start, "de", 2) || !STRNCASECMP(op0start, "hl", 2) || !STRNCASECMP(op0start, "ix", 2) || !STRNCASECMP(op0start, "iy", 2)))
        {
          if (!STRNCASECMP(op1start, "(hl)", 4))
            return(2);
          if (argCont(op1start, "(ix)") || argCont(op1start, "(iy)"))
            return(3);
        }
      if(IS_EZ80 &&
        (!STRNCASECMP(op1start, "bc", 2) || !STRNCASECMP(op1start, "de", 2) || !STRNCASECMP(op1start, "hl", 2) || !STRNCASECMP(op1start, "ix", 2) || !STRNCASECMP(op1start, "iy", 2)))
        {
          if (!STRNCASECMP(op0start, "(hl)", 4))
            return(2);
          if (argCont(op0start, "(ix)") || argCont(op0start, "(iy)"))
            return(3);
        }

      /* These 4 are the only remaining cases of 3 byte long ld instructions. */
      if(argCont(op1start, "(ix)") || argCont(op1start, "(iy)"))
        return(3);
      if(argCont(op0start, "(ix)") || argCont(op0start, "(iy)"))
        return(3);
      if((op0start[0] == '(' && STRNCASECMP(op0start, "(bc)", 4) && STRNCASECMP(op0start, "(de)", 4) && STRNCASECMP(op0start, "(hl", 3)) ||
         (op1start[0] == '(' && STRNCASECMP(op1start, "(bc)", 4) && STRNCASECMP(op1start, "(de)", 4) && STRNCASECMP(op1start, "(hl", 3)))
        return(3);
      if(op1start[0] == '#' &&
         (!STRNCASECMP(op0start, "bc", 2) || !STRNCASECMP(op0start, "de", 2) || !STRNCASECMP(op0start, "hl", 2) || !STRNCASECMP(op0start, "sp", 2)))
        return(3);

      /* These 3 are the only remaining cases of 2 byte long ld instructions. */
      if(op1start[0] == '#')
        return(2);
      if(!STRNCASECMP(op0start, "i", 1) || !STRNCASECMP(op0start, "r", 1) ||
         !STRNCASECMP(op1start, "i", 1) || !STRNCASECMP(op1start, "r", 1))
        return(2);
      if(!STRNCASECMP(op1start, "ix", 2) || !STRNCASECMP(op1start, "iy", 2))
        return(2);

      /* All other ld instructions */
      return(1);
    }

  // load from sp with offset
  if(IS_SM83 && (lineIsInst (pl, "lda") || lineIsInst (pl, "ldhl")))
    {
      return(2);
    }
  // load from/to 0xffXX addresses
  if(IS_SM83 && (lineIsInst (pl, "ldh") || lineIsInst (pl, "in") || lineIsInst (pl, "out")))
    {
      if(!STRNCASECMP(pl->line, "(c)", 3))
        return(1);
      return(2);
    }

  /* Exchange */
  if(lineIsInst (pl, "exx"))
    return(1);
  if(lineIsInst (pl, "ex"))
    {
      if(!op1start)
        {
          werrorfl(pl->ic->filename, pl->ic->lineno, W_UNRECOGNIZED_ASM, __func__, 4, pl->line);
          return(4);
        }
      if (argCont(op0start, "(sp)") && (IS_RAB || !STRNCASECMP(op1start, "ix", 2) || !STRNCASECMP(op1start, "iy", 2)))
        return(2);
      if ((IS_R4K || IS_R5K | IS_R6K) && (!STRNCASECMP (op0start, "bc", 2) || !STRNCASECMP (op0start, "jk", 2) || !STRNCASECMP (op0start, "bcde", 4)))
        return(2);
      return(1);
    }

  /* Push / pop */
  if(lineIsInst (pl, "push") && (IS_Z80N || IS_R4K || IS_R5K || IS_R6K) && op0start[0] == '#')
    return(4);
  if(lineIsInst (pl, "push") || lineIsInst (pl, "pop"))
    {
      if(!STRNCASECMP(op0start, "ix", 2) || !STRNCASECMP(op0start, "iy", 2) ||
        (IS_R4K || IS_R5K || IS_R6K) && (!STRNCASECMP(op0start, "bcde", 4) || !STRNCASECMP(op0start, "jkhl", 4)))
        return(2);
      return(1);
    }

  /* 16 bit add / subtract / and / or */
  if(IS_Z80N && lineIsInst (pl, "add") && (!STRNCASECMP(op0start, "bc", 2) || !STRNCASECMP(op0start, "de", 2) || !STRNCASECMP(op0start, "hl", 2))  && op1start[0] == '#')
    return(4);
  else if(IS_Z80N && lineIsInst (pl, "add") && (!STRNCASECMP(op0start, "bc", 2) || !STRNCASECMP(op0start, "de", 2) || !STRNCASECMP(op0start, "hl", 2)) && !STRNCASECMP(op1start, "a", 1))
    return(2);
  else if(IS_R6K && lineIsInst (pl, "add") && (!STRNCASECMP(op0start, "ix", 2) || !STRNCASECMP(op0start, "iy", 2))  && op1start[0] == '#')
    return(3);
  else if (IS_R6K && (lineIsInst (pl, "adc") || lineIsInst (pl, "add") || lineIsInst (pl, "and") || lineIsInst (pl, "cp") || lineIsInst (pl, "or") || lineIsInst (pl, "sbc") || lineIsInst (pl, "sub") || lineIsInst (pl, "xor")) && !STRNCASECMP(op0start, "hl", 2) && op1start && isdigit (op1start[0]))
    return(3);
  if((lineIsInst (pl, "add") || lineIsInst (pl, "adc") || lineIsInst (pl, "sbc") || IS_RAB && (lineIsInst (pl, "and") || lineIsInst (pl, "or")) ||
    (IS_R4K || IS_R5K || IS_R6K) && (lineIsInst (pl, "sub") || lineIsInst (pl, "cp"))) &&
     !STRNCASECMP(op0start, "hl", 2))
    {
      if(lineIsInst (pl, "cp")) // cp hl, de / cp hl, #d
        return(3);
      if(lineIsInst (pl, "add") || lineIsInst (pl, "and") || lineIsInst (pl, "or"))
        return(1);
      return(2);
    }
  if((lineIsInst (pl, "add") || IS_RAB && (lineIsInst (pl, "and") || lineIsInst (pl, "or")))&& (!STRNCASECMP(op0start, "ix", 2) || !STRNCASECMP(op0start, "iy", 2)))
    return(2);

  /* signed 8 bit adjustment to stack pointer */
  if((IS_RAB || IS_SM83) && lineIsInst (pl, "add") && !STRNCASECMP(op0start, "sp", 2))
    return(2);

  /* 16 bit adjustment to stack pointer */
  if(IS_TLCS90 && lineIsInst (pl, "add") && !STRNCASECMP(op0start, "sp", 2))
    return(3);

  /* 8 bit arithmetic, two operands */
  if(op1start && op0start[0] == 'a' &&
     (lineIsInst (pl, "add") || lineIsInst (pl, "adc") || lineIsInst (pl, "sub") || lineIsInst (pl, "sbc") ||
      lineIsInst (pl, "cp")  || lineIsInst (pl, "and") || lineIsInst (pl, "or")  || lineIsInst (pl, "xor")))
    {
      if(argCont(op1start, "(ix)") || argCont(op1start, "(iy)"))
        return(3);
      if(op1start[0] == '#')
        return(2);
      return(1);
    }
  /* 8 bit arithmetic, shorthand for a */
  if(!op1start &&
     (lineIsInst (pl, "add") || lineIsInst (pl, "adc") || lineIsInst (pl, "sub") || lineIsInst (pl, "sbc") ||
      lineIsInst (pl, "cp")  || lineIsInst (pl, "and") || lineIsInst (pl, "or")  || lineIsInst (pl, "xor")))
    {
      if(argCont(op0start, "(ix)") || argCont(op0start, "(iy)"))
        return(3);
      if(op0start[0] == '#')
        return(2);
      return(1);
    }

  if(lineIsInst (pl, "rlca") || lineIsInst (pl, "rla") || lineIsInst (pl, "rrca") || lineIsInst (pl, "rra"))
    return(1);

  /* Increment / decrement */
  if(lineIsInst (pl, "inc") || lineIsInst (pl, "dec"))
    {
      if(!STRNCASECMP(op0start, "ix", 2) || !STRNCASECMP(op0start, "iy", 2))
        return(2);
      if(argCont(op0start, "(ix)") || argCont(op0start, "(iy)"))
        return(3);
      return(1);
    }

  if(lineIsInst (pl, "rlc") || lineIsInst (pl, "rl")  || lineIsInst (pl, "rrc") || lineIsInst (pl, "rr") ||
     lineIsInst (pl, "sla") || lineIsInst (pl, "sra") || lineIsInst (pl, "srl"))
    {
      if(argCont(op0start, "(ix)") || argCont(op0start, "(iy)"))
        return(4);
      return(2);
    }

  if(lineIsInst (pl, "rld") || lineIsInst (pl, "rrd"))
    return(2);

  /* Bit */
  if(lineIsInst (pl, "bit") || lineIsInst (pl, "set") || lineIsInst (pl, "res"))
    {
      if(argCont(op1start, "(ix)") || argCont(op1start, "(iy)"))
        return(4);
      return(2);
    }

  if(lineIsInst (pl, "jr") || lineIsInst (pl, "djnz"))
    return(2);

  if(lineIsInst (pl, "jp"))
    {
      if(!STRNCASECMP(op0start, "(hl)", 4))
        return(1);
      if(!STRNCASECMP(op0start, "(ix)", 4) || !STRNCASECMP(op0start, "(iy)", 4))
        return(2);
      return(3);
    }

  if (IS_RAB &&
      (lineIsInst (pl, "ipset3") || lineIsInst (pl, "ipset2") ||
       lineIsInst (pl, "ipset1") || lineIsInst (pl, "ipset0") ||
       lineIsInst (pl, "ipres")))
    return(2);

  if(!IS_SM83 && (lineIsInst (pl, "reti") || lineIsInst (pl, "retn")))
    return(2);

  if(lineIsInst (pl, "ret") || lineIsInst (pl, "reti") || lineIsInst (pl, "rst"))
    return(1);

  if(lineIsInst (pl, "call"))
    return(3);

  /* Alias for ld a, (hl+)/(hld) etc */
  if(IS_SM83 && (lineIsInst (pl, "ldi") || lineIsInst (pl, "ldd")))
    return (1);

  if(lineIsInst (pl, "ldi") || lineIsInst (pl, "ldd") || lineIsInst (pl, "cpi") || lineIsInst (pl, "cpd"))
    return(2);

  if(lineIsInst (pl, "neg"))
    return(2);

  if(lineIsInst (pl, "daa") || lineIsInst (pl, "cpl")  || lineIsInst (pl, "ccf") || lineIsInst (pl, "scf") ||
     lineIsInst (pl, "nop") || lineIsInst (pl, "halt") || lineIsInst (pl,  "ei") || lineIsInst (pl, "di"))
    return(1);
  
  /* We always emit 2B stop due to hardware bugs*/
  if (IS_SM83 && lineIsInst (pl, "stop"))
    return(2);

  if(lineIsInst (pl, "im"))
    return(2);

  if(lineIsInst (pl, "in") || lineIsInst (pl, "out") || lineIsInst (pl, "ot") ||
     lineIsInst (pl, "ini") || lineIsInst (pl, "inir") || lineIsInst (pl, "ind") ||
     lineIsInst (pl, "indr") || lineIsInst (pl, "outi") || lineIsInst (pl, "otir") ||
     lineIsInst (pl, "outd") || lineIsInst (pl, "otdr"))
    {
      return(2);
    }

  if(lineIsInst (pl, "lddr") || lineIsInst (pl, "ldir") || lineIsInst (pl, "cpir") || lineIsInst (pl, "cpdr"))
    return(2);

  if((IS_Z180 || IS_EZ80) && (lineIsInst (pl, "in0") || lineIsInst (pl, "out0")))
    return(3);

  if((IS_Z180 || IS_EZ80 || IS_Z80N) && lineIsInst (pl, "mlt"))
    return(2);

  if (IS_R800 && (lineIsInst (pl, "multu") || lineIsInst (pl, "multuw")))
    return(2);

  if((IS_Z180 || IS_EZ80 || IS_Z80N) && lineIsInst (pl, "tst"))
    return((op0start[0] == '#' || op1start && op0start[0] == '#') ? 3 : 2);

  if((IS_R4K || IS_R5K || IS_R6K) &&
    (lineIsInst (pl, "clr") ||
    lineIsInst (pl, "test")))
    return(2);

  if((IS_R4K || IS_R5K || IS_R6K) &&
    lineIsInst (pl, "cbm"))
    return(3);
    
  if(IS_RAB && (lineIsInst (pl, "ioi") || lineIsInst (pl, "ioe")))
    return(1);

  if(IS_RAB && lineIsInst (pl, "ipset"))
    return(2);

  if(IS_RAB && lineIsInst (pl, "mul"))
    return(1);

  if((IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "mulu"))
    return(2);

  if((IS_R3KA || IS_R4K || IS_R5K || IS_R6K) &&
    (lineIsInst (pl, "lddsr") || lineIsInst (pl, "ldisr") ||
     lineIsInst (pl, "lsdr")  || lineIsInst (pl, "lsir")  ||
     lineIsInst (pl, "lsddr") || lineIsInst (pl, "lsidr")))
    return(2);

  if((IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "ldf"))
    return(5);

  if((IS_R3KA || IS_R4K || IS_R5K || IS_R6K) &&
    (lineIsInst (pl, "uma") || lineIsInst (pl, "ums")))
    return(2);

  if(IS_RAB && lineIsInst (pl, "bool"))
    return(!STRNCASECMP(op0start, "hl", 2) ? 1 : 2);

  if(IS_RAB && lineIsInst (pl, "lret") ||
    (IS_R4K || IS_R5K || IS_R6K) && lineIsInst (pl, "llret"))
    return(2);

  if(IS_RAB && lineIsInst (pl, "ldp"))
    {
      if (!strncmp (op0start, "(hl)", 4) || !strncmp (op0start, "(ix)", 4) || !strncmp (op0start, "(iy)", 4))
        return (2);
      if (!strncmp (op1start, "(hl)", 4) || !strncmp (op1start, "(ix)", 4) || !strncmp (op1start, "(iy)", 4))
        return (2);
      return(4);
    }

  if (IS_R6K && lineIsInst (pl, "swap"))
    return (2);

  if(IS_EZ80 && (lineIsInst (pl, "lea") || lineIsInst (pl, "pea")))
    return(3);

  if((IS_SM83 || IS_Z80N) && lineIsInst (pl, "swap"))
    return(2);

  if(IS_Z80N && (lineIsInst (pl, "swapnib") || lineIsInst (pl, "mirror") || lineIsInst (pl, "mul") ||
     lineIsInst (pl, "outinb") || lineIsInst (pl, "ldix") || lineIsInst (pl, "ldirx") ||
     lineIsInst (pl, "lddx") || lineIsInst (pl, "lddrx") || lineIsInst (pl, "ldpirx")))
    return(2);

  if(IS_Z80N &&
    (lineIsInst (pl, "bsla") ||
    lineIsInst (pl, "bsra") ||
    lineIsInst (pl, "bsrl") ||
    lineIsInst (pl, "bsrf") ||
    lineIsInst (pl, "brlc")))
    return(2);

  if(IS_Z80N && lineIsInst (pl, "nextreg"))
    return(op1start && !STRNCASECMP(op1start, "a", 1) ? 3 : 4);

  if(lineIsInst (pl, ".db") || lineIsInst (pl, ".byte"))
    {
      int i, j;
      for(i = 1, j = 0; pl->line[j]; i += pl->line[j] == ',', j++);
      return(i);
    }

  if(lineIsInst (pl, ".dw") || lineIsInst (pl, ".word"))
    {
      int i, j;
      for(i = 1, j = 0; pl->line[j]; i += pl->line[j] == ',', j++);
      return(i * 2);
    }
  
  if(IS_SM83 && lineIsInst (pl, ".tile"))
    {
      const char *value;
      int i;
      // skip directive
      for (value = pl->line; *value && !isspace (*value); ++value);
      // skip space
      for (; *value && isspace (*value); ++value);
      // just part of the syntax
      if(*value == '^')
        ++value;
      // delimiter can be freely chosen
      char delimiter = *(value++);
      for (i = 0; *value && *value != delimiter; ++value, ++i);
      // has to end with delimiter
      // 8 characters per 2B tile
      if (*value == delimiter && i%8 == 0)
        return i/4;
      
    }

  /* If the instruction is unrecognized, we shouldn't try to optimize.  */
  /* For all we know it might be some .ds or similar possibly long line */
  /* Return a large value to discourage optimization.                   */
  if (pl->ic)
    werrorfl(pl->ic->filename, pl->ic->lineno, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
  else
    werrorfl("unknown", 0, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
  return(999);
}

bool z80symmParmStack (const char *name)
{
  if (!strcmp (name, "___sdcc_enter_ix"))
   return false;
  return z80_symmParm_in_calls_from_current_function;
}

