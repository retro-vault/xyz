#include "common.h"
#include "SDCCicode.h"
#include "SDCCglobl.h"
#include "SDCCgen.h"

#include "peep.h"
#include "gen.h"

#define IS_F8L TARGET_IS_F8L

#define NOTUSEDERROR() do {werror(E_INTERNAL_ERROR, __FILE__, __LINE__, "error in notUsed()");} while(0)

// #define D(_s) { printf _s; fflush(stdout); }
#define D(_s)

#define STARTSINST(l, i) (!STRNCASECMP((l), (i), sizeof(i) - 1))

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

//extern bool stm8_regs_used_as_parms_in_calls_from_current_function[YH_IDX + 1];
//extern bool stm8_regs_used_as_parms_in_pcalls_from_current_function[YH_IDX + 1];

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

  /* In each jump the label is at the end */
  p = strlen (pl->line) - 1 + pl->line;

  /* Skip trailing whitespace */
  while(isspace(*p))
    p--;

  /* scan backward until space or ',' */
  for (; p > pl->line; p--)
    if (isspace(*p) || *p == ',' || *p == '#')
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

static bool isReg8(const char *arg)
{
  return (!strncmp (arg, "xl", 2) || !strncmp (arg, "xh", 2) ||
    !strncmp (arg, "yl", 2) || !strncmp (arg, "yh", 2) ||
    !strncmp (arg, "zl", 2) || !strncmp (arg, "zh", 2));
}

static bool isReg16(const char *arg)
{
  return ((arg[0] == 'x' || arg[0] == 'y' || arg[0] == 'z') && !isalnum(arg[1]));
}

static bool isSprel(const char *arg)
{
  if (*arg != '(')
    return false;
  arg++;
  for(;; arg++)
    {
      if (!*arg)
        return false;
      if (*arg == ',')
        break;
    }
  return (!strncmp (arg, ", sp)", 5));
}

static bool isYrel(const char *arg)
{
  if (*arg != '(')
    return false;
  arg++;
  for(;; arg++)
    {
      if (!*arg)
        return false;
      if (*arg == ',')
        break;
    }
  return (!strncmp (arg, ", y)", 4));
}

static bool
isInt(const char *str)
{
  int ret;
  while(str[0] == '#' || str[0] == '(' || str[0] == '[' || isspace ((unsigned char)str[0]))
    str++;
  if(sscanf(str, "0x%x", &ret))
    return(ret);
  if(!sscanf(str, "%d", &ret))
    return(false);
  return(true);
}

static int
readint(const char *str)
{
  int ret;
  while(str[0] == '#' || str[0] == '(' || str[0] == '[' || isspace ((unsigned char)str[0]))
    str++;
  if(sscanf(str, "0x%x", &ret))
    return(ret);
  if(!sscanf(str, "%d", &ret))
    {
      wassertl (0, "readint() got non-integer argument:");
      fprintf (stderr, "%s\n", str);
      ret = -1;
    }
  return(ret);
}

int
f8instructionSize (lineNode *pl)
{
  const char *larg = lineArg (pl, 0);
  const char *rarg = lineArg (pl, 1);

  if (IS_F8L && (larg && isYrel (larg) || rarg && isYrel (rarg))) // f8l doesn not have y-relative addressing.
    if (pl->ic)
      werrorfl(pl->ic->filename, pl->ic->lineno, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
    else
      werrorfl("unknown", 0, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);

  if (larg &&
    (lineIsInst (pl, "adc") || lineIsInst (pl, "add") || lineIsInst (pl, "and") ||  lineIsInst (pl, "cp") ||
    lineIsInst (pl, "or") || lineIsInst (pl, "sbc") || lineIsInst (pl, "sub") || lineIsInst (pl, "xor")))
    {
      if (!strncmp (larg, "xl", 2) && isReg8 (rarg))
        return 1;
      else if (isReg8 (larg) && !strncmp (rarg, "xl", 2))
        return 2;
      else if (!strncmp (larg, "xl", 2) && isSprel (rarg))
        return 2;
      else if (isReg8 (larg) && isSprel (rarg))
        return 3;
      else if (!strncmp (larg, "xl", 2) && rarg[0] == '#')
        return 2;
      else if (isReg8 (larg) && rarg[0] == '#')
        return 3;
      else if (!strncmp (larg, "xl", 2) )
        return 3;
      else
        return 4;
    }

  if (larg &&
    (lineIsInst (pl, "bool") || lineIsInst (pl, "clr") || lineIsInst (pl, "daa") || lineIsInst (pl, "dec") ||
    lineIsInst (pl, "inc") ||  lineIsInst (pl, "pop") ||  lineIsInst (pl, "push") ||  lineIsInst (pl, "rlc") ||
    lineIsInst (pl, "rrc") || lineIsInst (pl, "sll") || lineIsInst (pl, "sra") || lineIsInst (pl, "srl") ||
    lineIsInst (pl, "thrd") || lineIsInst (pl, "tst")))
    {
      if (!strncmp (larg, "xl", 2))
        return 1;
      else if (isReg8 (larg))
        return 2;
      else if (larg[0] == '#' || isSprel (larg) || isYrel (larg))
        return 2;
      else
        return 3;
    }

  if (larg &&
    (!IS_F8L && lineIsInst (pl, "adcw") || !IS_F8L && lineIsInst (pl, "sbcw") && (!rarg || !rarg[0]) ||
    !IS_F8L && lineIsInst (pl, "boolw") || lineIsInst (pl, "clrw") || !IS_F8L && lineIsInst (pl, "decw") || lineIsInst (pl, "incw") ||
    !IS_F8L && lineIsInst (pl, "incnw") || !IS_F8L && lineIsInst (pl, "mul") || !IS_F8L && lineIsInst (pl, "negw") || lineIsInst (pl, "popw") ||
    lineIsInst (pl, "pushw") || !IS_F8L && lineIsInst (pl, "sllw") || !IS_F8L && lineIsInst (pl, "sraw") || !IS_F8L && lineIsInst (pl, "srlw") ||
    !IS_F8L && lineIsInst (pl, "rlcw") || !IS_F8L && lineIsInst (pl, "rrcw") || lineIsInst (pl, "tstw")))
    {
      if (larg[0] == 'y')
        return 1;
      else if (isReg16 (larg))
        return 2;
      else if (isSprel (larg))
        return 2;
      else
        return 3;
    }

  if (larg && lineIsInst (pl, "xch") && larg[0] == 'f' && isSprel (rarg))
    return 1;
  else if (larg && lineIsInst (pl, "xch"))
    return 1 + (larg[0] != 'y');
  
  if (lineIsInst (pl, "addw") && !strncmp (larg, "sp", 2) && rarg[0] == '#')
    return 2;

  if (lineIsInst (pl, "addw") && rarg[0] == '#' && isInt (rarg) && readint (rarg) <= 0xff)
    {
      if (larg[0] == 'y')
        return 2;
      else
        return 3;
    }

  if (!IS_F8L && larg && (lineIsInst (pl, "addw") || lineIsInst (pl, "cpw") || lineIsInst (pl, "orw") || lineIsInst (pl, "sbcw") ||
    lineIsInst (pl, "subw") || lineIsInst (pl, "xorw")))
    {
      if (larg[0] == 'y' && rarg[0] == 'x')
        return 1;
      else if (isReg16 (larg) && isReg16 (rarg))
        return 2;
      else if (larg[0] == 'y' && isSprel (rarg))
        return 2;
      else if (isReg16 (larg) && isSprel (rarg))
        return 3;
      else if (larg[0] == 'y')
        return 3;
      else
        return 4;
    }

  if (larg && lineIsInst (pl, "ld"))
    {
      if (!strncmp (larg, "xl", 2) && isReg8 (rarg))
        return 1;
      else if (isReg8 (larg) && !strncmp (rarg, "xl", 2))
        return 2;
      else if (!strncmp (larg, "xl", 2) && (isSprel (rarg) || isYrel (rarg)) ||
        (isSprel (larg) || isYrel (larg)) && !strncmp (rarg, "xl", 2))
        return 2;
      else if (isReg8 (larg) && (isSprel (rarg) || isYrel (rarg)) ||
        (isSprel (larg) || isYrel (larg)) && isReg8 (rarg))
        return 3;
      else if (!strncmp (larg, "xl", 2) && rarg[0] == '#')
        return 2;
      else if (!strncmp (larg, "xl", 2) && !strncmp (rarg, "(y)", 3))
        return 1;
      else if (isReg8 (larg) && !strncmp (rarg, "(y)", 3))
        return 2;
      else if (!strncmp (larg, "(y)", 3) && !strncmp (rarg, "xl", 2))
        return 1;
      else if (!strncmp (larg, "(y)", 3) && isReg8 (rarg))
        return 2;
      else if (isReg8 (larg) && isReg8 (rarg))
        return 2;
      else if (!strncmp (larg, "xl", 2) || !strncmp (rarg, "xl", 2))
        return 3;
      else
        return 4;
    }

  if (!IS_F8L && larg && (lineIsInst (pl, "ldi") || lineIsInst (pl, "ldwi")) && isYrel (larg))
    return 2;

  if (larg && lineIsInst (pl, "ldw"))
    {
      if (larg[0] == 'y' && rarg[0] == 'x')
        return 1;
      else if (isReg16 (larg) && rarg[0] == 'y')
        return 1;
      else if (larg[0] == 'y' && rarg[0] == 'z')
        return 2;
      else if (larg[0] == 'y' && (isSprel (rarg) || isYrel (rarg)) ||
        isSprel (larg) && rarg[0] == 'y')
        return 2;
      else if (isReg16 (larg) && isSprel (rarg) ||
        isSprel (larg) && isReg16 (rarg))
        return 3;
      else if (larg[0] == 'y' && !strncmp (rarg, "(y)", 3))
        return 1;
      else if (!strncmp (larg, "(y)", 3) && rarg[0] == 'x')
        return 1;
      else if (isYrel (larg) && rarg[0] == 'x')
        return 2;
      else if (larg[0] == 'y' && rarg[0] == '#' && isInt (rarg) && readint (rarg) <= 0xff)
        return 2;
      else if (isReg16 (larg) && rarg[0] == '#' && isInt (rarg) && readint (rarg) <= 0xff)
        return 3;
      else if (larg[0] == 'y' || rarg[0] == 'y')
        return 3;
      else
        return 4;
    }

  if (!IS_F8L && larg && lineIsInst (pl, "rot"))
    {
      if (!strncmp (larg, "xl", 2))
        return 2;
      else
        return 3;
    }

  if (!IS_F8L && larg && (lineIsInst (pl, "zex") || lineIsInst (pl, "sex")))
    {
      if (!strncmp (larg, "y", 1) && !strncmp (rarg, "xl", 2))
        return 1;
      else
        return 2;
    }

  if (!IS_F8L && lineIsInst (pl, "mad"))
    return 1;

  if (larg && (lineIsInst (pl, "call") || lineIsInst (pl, "jp")))
    {
      if (larg[0] == 'y')
        return 1;
      else if (isReg16 (larg))
        return 2;
      else
        return 3;
    }
  else if (STARTSINST (pl->line, "jr"))
    {
      if (lineIsInst (pl, "jro") || lineIsInst (pl, "jrsgt") || lineIsInst (pl, "jrgt"))
        return 3;
      else
        return 2;
    }
  else if (!IS_F8L && larg && lineIsInst (pl, "dnjnz"))
    {
      if (!strncmp (larg, "yh", 2))
        return 2;
      else
        return 3;
    }
  else if (lineIsInst (pl, "ret") || lineIsInst (pl, "reti"))
    return 1;

  // If the instruction is unrecognized, we shouldn't try to optimize.
  // For all we know it might be some .ds or similar possibly long line
  // Return a large value to discourage optimization.
  if (pl->ic)
    werrorfl(pl->ic->filename, pl->ic->lineno, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
  else
    werrorfl("unknown", 0, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
  return(999);
}

static bool
f8MightReadFlag (const lineNode *pl, const char *what)
{
  if (lineIsInst (pl, "adc") || lineIsInst (pl, "sbc"))
    return !strcmp (what, "cf");
  if (lineIsInst (pl, "adcw") || lineIsInst (pl, "sbcw"))
    return !strcmp (what, "cf");
  if (lineIsInst (pl, "add") || lineIsInst (pl, "and") || lineIsInst (pl, "cp") || lineIsInst (pl, "or") || lineIsInst (pl, "sub") || lineIsInst (pl, "xor"))
    return false;
  if (lineIsInst (pl, "sll") || lineIsInst (pl, "srl") || lineIsInst (pl, "inc") || lineIsInst (pl, "dec") || lineIsInst (pl, "clr") || lineIsInst (pl, "push") || lineIsInst (pl, "tst"))
    return false;
  if (lineIsInst (pl, "rlc") || lineIsInst (pl, "rrc"))
    return !strcmp (what, "cf");
  if (lineIsInst (pl, "addw") || lineIsInst (pl, "orw") || lineIsInst (pl, "subw"))
    return false;
  if (lineIsInst (pl, "adcw") || lineIsInst (pl, "rlcw") || lineIsInst (pl, "rrcw") || lineIsInst (pl, "sbcw"))
    return !strcmp (what, "cf");
  if (lineIsInst (pl, "clrw") || lineIsInst (pl, "incw") || lineIsInst (pl, "pushw") || lineIsInst (pl, "sraw") || lineIsInst (pl, "srlw") || lineIsInst (pl, "sllw") || lineIsInst (pl, "tstw"))
    return false;
  if (lineIsInst (pl, "ld") || lineIsInst (pl, "ldw") || lineIsInst (pl, "ldi") || lineIsInst (pl, "ldwi"))
    return false;
  if (lineIsInst (pl, "bool") || lineIsInst (pl, "cax") || lineIsInst (pl, "mad") || lineIsInst (pl, "msk") || lineIsInst (pl, "pop") || lineIsInst (pl, "rot") || lineIsInst (pl, "sra") || lineIsInst (pl, "thrd"))
    return false;
  if (lineIsInst (pl, "daa"))
    return (!strcmp (what, "cf") || !strcmp (what, "hf"));
  if (lineIsInst (pl, "xch"))
    return (lineArg (pl, 0)[0] == 'f');
  if (lineIsInst (pl, "boolw") || lineIsInst (pl, "caxw") || lineIsInst (pl, "cpw") || lineIsInst (pl, "decw") || lineIsInst (pl, "incnw") || lineIsInst (pl, "mul") || lineIsInst (pl, "negw") || lineIsInst (pl, "popw") || lineIsInst (pl, "sex") || lineIsInst (pl, "xchw") || lineIsInst (pl, "zex"))
    return false;
  if (lineIsInst (pl, "xchb"))
    return false;

  if (lineIsInst (pl, "call") || lineIsInst (pl, "dnjnz") || lineIsInst (pl, "jr") || lineIsInst (pl, "jp"))
    return false;
  if (lineIsInst (pl, "jrc") || lineIsInst (pl, "jrnc"))
    return !strcmp (what, "cf");
  if (lineIsInst (pl, "jrn") || lineIsInst (pl, "jrnn"))
    return !strcmp (what, "nf");
  if (lineIsInst (pl, "jrz") || lineIsInst (pl, "jrnz"))
    return !strcmp (what, "zf");
  if (lineIsInst (pl, "jrno") || lineIsInst (pl, "jro"))
    return !strcmp (what, "of");
  if (lineIsInst (pl, "jrsle") || lineIsInst (pl, "jrsgt"))
    return !strcmp (what, "zf") || !strcmp (what, "nf") || !strcmp (what, "of");
  if (lineIsInst (pl, "jrle") || lineIsInst (pl, "jrgt"))
    return !strcmp (what, "cf") || !strcmp (what, "zf");

  if (lineIsInst (pl, "ret"))
    return false;
  if (lineIsInst (pl, "reti"))
    return true;
  if (lineIsInst (pl, "nop"))
    return false;

  if (pl->ic)
    werrorfl(pl->ic->filename, pl->ic->lineno, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
  else
    werrorfl("unknown", 0, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);

  // Fail-safe fallback.
  return true;
}

/* Check if reading arg implies reading what. */
static bool argCont(const char *arg, char what)
{
  if (!arg || strlen (arg) == 0 || !(what == 'x' || what == 'y' || what == 'z'))
    return false;

  while (isblank ((unsigned char)(arg[0])))
    arg++;

  if (arg[0] == ',')
    arg++;

  while (isblank ((unsigned char)(arg[0])))
    arg++;

  if (arg[0] == '#')
    return false;

  if (arg[0] == '(') 
    arg++;
  if (arg[0] == '0' && (tolower(arg[1])) == 'x') 
    arg += 2; // Skip hex prefix to avoid false x positive.

  if (strlen(arg) == 0)
    return false;

  if (arg[0] == '_')
    {
      arg = strchr(arg, ','); // Skip over user-defined variable names.
      if (!arg)
        return false;
    }

  while (arg[0])
    {
      if (arg[0] == ';') // Start of end-of-line comment
        return false;
      if (arg[0] == what)
        return true;
      if (arg[0] == '#') // Skip lit prefix
        do
          arg++;
        while (isalnum (arg[0]));
      else
        arg++;
    }

  return false;
}

static bool
f8MightRead (const lineNode *pl, const char *what)
{
  char extra = 0;

  if (!strcmp (what, "xl") || !strcmp (what, "xh"))
    extra = 'x';
  else if (!strcmp (what, "yl") || !strcmp (what, "yh"))
    extra = 'y';
  else if (!strcmp (what, "zl") || !strcmp (what, "zh"))
    extra = 'z';
  else
    return f8MightReadFlag(pl, what);

  if (lineIsInst (pl, "jp") || lineIsInst (pl, "jr"))
    return false;

  // 8-bit 2-op inst, and some others.
  if (lineIsInst (pl, "adc") || lineIsInst (pl, "add") || lineIsInst (pl, "and") || lineIsInst (pl, "cp") || lineIsInst (pl, "or") || lineIsInst (pl, "sbc") || lineIsInst (pl, "sub") || lineIsInst (pl, "xch") || lineIsInst (pl, "xor"))
    {
      const char *larg = lineArg (pl, 0);
      const char *rarg = lineArg (pl, 1);

      if (larg[0] == what[0] && larg[1] == what[1] || argCont (larg + 1, extra))
        return true;
      if (rarg && (rarg[0] == what[0] && rarg[1] == what[1] || argCont (rarg + 1, extra)))
        return true;
      return false;
    }
  // 8-bit 1-op inst, and some others
  if (lineIsInst (pl, "pop"))
    return false;
  if (lineIsInst (pl, "clr"))
    {
      const char *larg = lineArg (pl, 0);
      return (larg[0] == '(' && argCont (larg, extra));
    }
  if (lineIsInst (pl, "bool") || lineIsInst (pl, "dec") || lineIsInst (pl, "inc") || lineIsInst (pl, "push") || lineIsInst (pl, "rlc") || lineIsInst (pl, "rot") || lineIsInst (pl, "rrc") || lineIsInst (pl, "sll") || lineIsInst (pl, "sra") || lineIsInst (pl, "srl") || lineIsInst (pl, "tst") || lineIsInst (pl, "xchb"))
    {
      const char *larg = lineArg (pl, 0);

      return (larg[0] == what[0] && larg[1] == what[1] || argCont (larg + 1, extra));
    }
  // 16-bit 2/1-op inst, and some others.
  if (lineIsInst (pl, "clrw") || lineIsInst (pl, "popw"))
    return false;
  if (lineIsInst (pl, "adcw") || lineIsInst (pl, "addw") || lineIsInst (pl, "boolw") || lineIsInst (pl, "cpw") || lineIsInst (pl, "decw") || lineIsInst (pl, "incw") || lineIsInst (pl, "mul") || lineIsInst (pl, "negw") || lineIsInst (pl, "orw") || lineIsInst (pl, "pushw") || lineIsInst (pl, "rlcw") || lineIsInst (pl, "rrcw") || lineIsInst (pl, "sllw") || lineIsInst (pl, "sraw") || lineIsInst (pl, "srlw") || lineIsInst (pl, "subw") || lineIsInst (pl, "sbcw") || lineIsInst (pl, "tstw") || lineIsInst (pl, "incnw") || lineIsInst (pl, "xorw"))
    {
      const char *larg = lineArg (pl, 0);
      const char *rarg = lineArg (pl, 1);

      if (argCont (larg, extra))
        return true;
      if (rarg && argCont (rarg, extra))
        return true;
      return false;
    }
  // ld
  if (lineIsInst (pl, "ld"))
    {
      const char *larg = lineArg (pl, 0);
      const char *rarg = lineArg (pl, 1);

      if (rarg && (rarg[0] == what[0] && rarg[1] == what[1] || argCont (rarg + 1, extra)))
        return true;
      if (larg[0] == what[0] && larg[1] == what[1])
        return false;
      if (argCont (larg + 1, extra))
        return true;
      return false;
    }
  // ldw
  if (lineIsInst (pl, "ldi") || lineIsInst (pl, "ldwi"))
    return (extra == 'y' || extra == 'z');
  if (lineIsInst (pl, "ldw"))
    {
      const char *larg = lineArg (pl, 0);
      const char *rarg = lineArg (pl, 1);

      if (rarg && (rarg[0] == what[0] && rarg[1] == what[1] || argCont (rarg + 1, extra)))
        return true;
      if (larg[0] == what[0] && larg[1] == what[1])
        return false;
      if (argCont (larg + 1, extra))
        return true;
      return false;
    }
  if (lineIsInst (pl, "sex") || lineIsInst (pl, "zex"))
    {
      const char *rarg = lineArg (pl, 1);
      if (rarg && (rarg[0] == what[0] && rarg[1] == what[1]))
        return true;
      return false;
    }
  if (lineIsInst (pl, "call"))
    {
      const char *larg = lineArg (pl, 0);
      if (*larg == '#')
        larg++;
      if (*larg == '_')
        larg++;
      const symbol *f = findSym (SymbolTab, 0, larg);
      if (*larg == extra)
        return true;
      if (f && IS_FUNC (f->type))
        return f8IsParmInCall (f->type, what);
      else // Fallback needed for calls through function pointers and for calls to literal addresses.
        return true; // todo: improve accuracy here.
    }
  if (STARTSINST (pl->line, "jr"))
    return false;
  if (lineIsInst (pl, "ret"))
    return f8IsReturned(what);
  if (lineIsInst (pl, "reti"))
    return true;

  if (pl->ic)
    werrorfl(pl->ic->filename, pl->ic->lineno, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
  else
    werrorfl("unknown", 0, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);

  // Fail-safe fallback.
  return true;
}

static bool
f8UncondJump (const lineNode *pl)
{
  return (lineIsInst (pl, "jp") || lineIsInst (pl, "jr"));
}

static bool
f8CondJump (const lineNode *pl)
{
  return (!f8UncondJump (pl) && STARTSINST (pl->line, "jr") ||
    lineIsInst (pl, "dnjz"));
}

static bool
f8SurelyWritesFlag (const lineNode *pl, const char *what)
{
  // 8-bit 2-op inst.
  if (lineIsInst (pl, "adc") || lineIsInst (pl, "add") || lineIsInst (pl, "cp") || lineIsInst (pl, "sbc") || lineIsInst (pl, "sub"))
    return true;
  if (lineIsInst (pl, "or") || lineIsInst (pl, "and") || lineIsInst (pl, "xor"))
    return (!strcmp (what, "zf") || !strcmp (what, "nf"));
  // 8-bit 1-op inst.
  if (lineIsInst (pl, "dec") || lineIsInst (pl, "inc"))
    return true;
  if (lineIsInst (pl, "clr") || lineIsInst (pl, "push"))
    return false;
  if (lineIsInst (pl, "srl") || lineIsInst (pl, "sll") || lineIsInst (pl, "rrc") || lineIsInst (pl, "rlc"))
    return (!strcmp (what, "zf") || !strcmp (what, "cf"));
  if (lineIsInst (pl, "tst"))
    return strcmp (what, "hf");
  // 16-bit 1/2-op inst.
  if (lineIsInst (pl, "adcw") || lineIsInst (pl, "cpw") || lineIsInst (pl, "decw") || lineIsInst (pl, "incw") || lineIsInst (pl, "negw") || lineIsInst (pl, "sbcw") || lineIsInst (pl, "subw"))
    return strcmp (what, "hf");
  if (lineIsInst (pl, "addw"))
    {
      const char *arg = lineArg (pl, 0);
      if (!strncmp (arg, "sp", 2))
        return false;
      else
        return strcmp (what, "hf");
    }
  if (lineIsInst (pl, "clrw") || lineIsInst (pl, "pushw"))
    return false;
  if (lineIsInst (pl, "orw"))
    return (!strcmp (what, "of") || !strcmp (what, "zf") || !strcmp (what, "nf"));
  if (lineIsInst (pl, "tstw"))
    return strcmp (what, "hf");
  // ld / ldw
  if (lineIsInst (pl, "ld") || lineIsInst (pl, "ldw"))
    {
      const char *rarg = lineArg (pl, 1);
      const char *rest = strstr (rarg, ", ");
      if (!rest || strchr (rarg, '#') && strchr (rarg, '#') < rest || strchr (rarg, ';') && strchr (rarg, ';') < rest)
        return false; // todo: ld xl, mm / ldw y, mm
      if (!strncmp (rest, ", y)", 2) || !strncmp (rest, ", z)", 2) || !strncmp (rest, ", sp)", 3))
        return (!strcmp (what, "zf") || !strcmp (what, "nf"));
      return false;
    }
  if (lineIsInst (pl, "ldi") || lineIsInst (pl, "ldwi"))
    return (!strcmp (what, "zf") || !strcmp (what, "nf"));
  // 8-bit 0-op inst.
  if (lineIsInst (pl, "bool") || lineIsInst (pl, "cax"))
    return !strcmp (what, "zf");
  if (lineIsInst (pl, "daa") || lineIsInst (pl, "sra"))
    return (!strcmp (what, "zf") || !strcmp (what, "cf"));
  if (lineIsInst (pl, "mad"))
    return (!strcmp (what, "zf") || !strcmp (what, "nf"));
  if (lineIsInst (pl, "msk") || lineIsInst (pl, "pop"))
    return false;
  if (lineIsInst (pl, "rot"))
    return false;
  if (lineIsInst (pl, "xch"))
    return lineArg (pl, 0)[0] == 'f';
  // 16-bit 0-op inst.
  if (lineIsInst (pl, "boolw") || lineIsInst (pl, "zex"))
    return !strcmp (what, "zf");
  if (lineIsInst (pl, "incnw") || lineIsInst (pl, "popw") || lineIsInst (pl, "xchw"))
    return false;
  if (lineIsInst (pl, "mul") || lineIsInst (pl, "sraw") || lineIsInst (pl, "srlw") || lineIsInst (pl, "rlcw") || lineIsInst (pl, "rrcw"))
    return (!strcmp (what, "zf") || !strcmp (what, "nf") || !strcmp (what, "cf"));
  if (lineIsInst (pl, "sex"))
    return (!strcmp (what, "zf") || !strcmp (what, "nf"));
  if (lineIsInst (pl, "sllw"))
    {
      if (!strcmp (what, "zf") || !strcmp (what, "nf"))
        return true;
      if (!lineArg (pl, 1) && !strcmp (what, "cf"))
        return true;
      return false;
    }
  if (lineIsInst (pl, "xchb"))
    return !strcmp (what, "zf");
  // jumps
  if (STARTSINST (pl->line, "jr"))
    return false;
  if (lineIsInst (pl, "jp")) // todo: improve accuracy by checking for function call vs. local jump.
    return false;
  if (lineIsInst (pl, "call"))
    return true;
  if (lineIsInst (pl, "ret"))
    return true;
  if (lineIsInst (pl, "reti"))
    return false;
  if (lineIsInst (pl, "nop"))
    return false;

  if (pl->ic)
    werrorfl(pl->ic->filename, pl->ic->lineno, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
  else
    werrorfl("unknown", 0, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);

  // Fail-safe fallback.
  return false;
}

static bool
f8SurelyWrites (const lineNode *pl, const char *what)
{
  char extra = 0;

  if (!strcmp (what, "xl") || !strcmp (what, "xh"))
    extra = 'x';
  else if (!strcmp (what, "yl") || !strcmp (what, "yh"))
    extra = 'y';
  else if (!strcmp (what, "zl") || !strcmp (what, "zh"))
    extra = 'z';
  else
    return (f8SurelyWritesFlag (pl, what));

  // 8-bit 1/2-op inst, and some others.
  if (lineIsInst (pl, "push") || lineIsInst (pl, "tst"))
     return false;
  if (lineIsInst (pl, "adc") || lineIsInst (pl, "add") || lineIsInst (pl, "and") || lineIsInst (pl, "bool") || lineIsInst (pl, "clr") || lineIsInst (pl, "dec") || lineIsInst (pl, "cp") || lineIsInst (pl, "inc") || lineIsInst (pl, "or") || lineIsInst (pl, "pop") || lineIsInst (pl, "rlc") || lineIsInst (pl, "rot") || lineIsInst (pl, "rrc") || lineIsInst (pl, "sbc") || lineIsInst (pl, "sub") || lineIsInst (pl, "sll") || lineIsInst (pl, "sra") || lineIsInst (pl, "srl") || lineIsInst (pl, "xor"))
    {
      const char *larg = lineArg (pl, 0);
      return (larg[0] == what[0] && larg[1] == what[1]);
    }
  // 16-bit 2/1-op inst, and some others.
  if (lineIsInst (pl, "pushw") || lineIsInst (pl, "tstw"))
    return false;
  if (lineIsInst (pl, "adcw") || lineIsInst (pl, "addw") || lineIsInst (pl, "boolw") || lineIsInst (pl, "clrw") || lineIsInst (pl, "cpw") || lineIsInst (pl, "decw") || lineIsInst (pl, "incw") || lineIsInst (pl, "mul") || lineIsInst (pl, "negw") || lineIsInst (pl, "orw") || lineIsInst (pl, "popw") || lineIsInst (pl, "rlcw") || lineIsInst (pl, "rrcw") || lineIsInst (pl, "sex") || lineIsInst (pl, "sllw") || lineIsInst (pl, "sraw") || lineIsInst (pl, "srlw") || lineIsInst (pl, "subw") || lineIsInst (pl, "sbcw") || lineIsInst (pl, "incnw") || lineIsInst (pl, "xorw") || lineIsInst (pl, "zex"))
    {
      const char *larg = lineArg (pl, 0);
      return (larg[0] == extra);
    }
  if (lineIsInst (pl, "ld"))
    return (pl->line[3] == what[0] && pl->line[4] == what[1]);
  if (lineIsInst (pl, "ldw"))
    return (pl->line[4] == extra);
  if (lineIsInst (pl, "ldi") || lineIsInst (pl, "ldwi"))
    return false;
  if (lineIsInst (pl, "xch"))
    {
      const char *larg = lineArg (pl, 0);
      const char *rarg = lineArg (pl, 1);
      return (larg[0] == what[0] && larg[1] == what[1] || rarg[0] == what[0] && rarg[1] == what[1]);
    }
  if (STARTSINST (pl->line, "jr"))
    return false;
  if (lineIsInst (pl, "jp") || lineIsInst (pl, "call"))
    return false; // Todo: Improve accuracy?
  if (lineIsInst (pl, "ret") || lineIsInst (pl, "reti"))
    return true;

  if (pl->ic)
    werrorfl(pl->ic->filename, pl->ic->lineno, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);
  else
    werrorfl("unknown", 0, W_UNRECOGNIZED_ASM, __func__, 999, pl->line);

  // Fail-safe fallback.
  return false;
}

static bool
f8SurelyReturns (const lineNode *pl)
{
  return (false);
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
  for (; *pl; *pl = (*pl)->next)
    {
      if (!(*pl)->line || (*pl)->isDebug || (*pl)->isComment || (*pl)->isLabel)
        continue;
      D(("Scanning %s for %s\n", (*pl)->line, what));
      /* don't optimize across inline assembler,
         e.g. isLabel doesn't work there */
      if ((*pl)->isInline)
        {
          D(("S4O_ABORT at inline asm\n"));
          return S4O_ABORT;
        }

      if ((*pl)->visited)
        {
          D(("S4O_VISITED\n"));
          return S4O_VISITED;
        }

      (*pl)->visited = TRUE;

      if(f8MightRead(*pl, what))
        {
          D(("S4O_RD_OP\n"));
          return S4O_RD_OP;
        }

      // Check writes before conditional jumps, some jumps (btjf, btjt) write 'c'
      if(f8SurelyWrites(*pl, what))
        {
          D(("S4O_WR_OP\n"));
          return S4O_WR_OP;
        }

      if(f8UncondJump(*pl))
        {
          *pl = findLabel (*pl);
            if (!*pl)
              {
                D(("S4O_ABORT at unconditional jump\n"));
                return S4O_ABORT;
              }
        }
      if(f8CondJump(*pl))
        {
          *plCond = findLabel (*pl);
          if (!*plCond)
            {
              D(("S4O_ABORT at conditional jump\n"));
              return S4O_ABORT;
            }
          D(("S4O_CONDJMP\n"));
          return S4O_CONDJMP;
        }

      if(f8SurelyReturns(*pl))
        {
          D(("S4O_TERM\n"));
          return S4O_TERM;
        }
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
            return true;
          case S4O_CONDJMP:
            /* two possible destinations: recurse */
              {
                lineNode *pl2 = plConditional;
                D(("CONDJMP trying other branch first\n"));
                if (!doTermScan (&pl2, what))
                  return false;
                D(("Other branch OK.\n"));
              }
            continue;
          case S4O_RD_OP:
          default:
            /* no go */
            return false;
        }
    }
}

/*-----------------------------------------------------------------*/
/* univisitLines - clear "visited" flag in all lines               */
/*-----------------------------------------------------------------*/
static void
unvisitLines (lineNode *pl)
{
  for (; pl; pl = pl->next)
    pl->visited = false;
}

bool
f8notUsed (const char *what, lineNode *endPl, lineNode *head)
{
  lineNode *pl;
  if (!strcmp(what, "x"))
    return(f8notUsed("xl", endPl, head) && f8notUsed("xh", endPl, head));
  else if (!strcmp(what, "y"))
    return(f8notUsed("yl", endPl, head) && f8notUsed("yh", endPl, head));
  else if (!strcmp(what, "z"))
    return(f8notUsed("zl", endPl, head) && f8notUsed("zh", endPl, head));

  // Only handle general-purpose registers and documented flags.
  if (!strcmp(what, "xl") || !strcmp(what, "xh") || !strcmp(what, "yl") || !strcmp(what, "yh") || !strcmp(what, "zl") || !strcmp(what, "zh") ||
    !strcmp(what, "of") || !strcmp(what, "zf") || !strcmp(what, "nf") || !strcmp(what, "cf") || !strcmp(what, "hf"))
    ;
  else
    return false;

  _G.head = head;

  unvisitLines (_G.head);

  pl = endPl->next;
  return (doTermScan (&pl, what));
}

bool
f8notUsedFrom (const char *what, const char *label, lineNode *head)
{
  lineNode *cpl;

  for (cpl = head; cpl; cpl = cpl->next)
    if (cpl->isLabel && !strncmp (label, cpl->line, strlen(label)))
      return (f8notUsed (what, cpl, head));

  return false;
}

/* can be directly assigned with ld */
bool
f8canAssign (const char *op1, const char *op2, const char *exotic)
{
  return false;
}

