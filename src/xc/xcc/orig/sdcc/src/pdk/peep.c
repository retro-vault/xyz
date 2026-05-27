#include "common.h"
#include "SDCCgen.h"

#include "peep.h"

#define NOTUSEDERROR() do {werror(E_INTERNAL_ERROR, __FILE__, __LINE__, "error in notUsed()");} while(0)

// #define D(_s) { printf _s; fflush(stdout); }
#define D(_s)

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

static bool
isReturned(const char *what)
{
  symbol *sym;
  sym_link *sym_lnk;
  int size;
  lineNode *l;

  l = _G.head;
  do
  {
    l = l->next;
  } while(l->isComment || l->ic == NULL || l->ic->op != FUNCTION);

  sym = OP_SYMBOL(IC_LEFT(l->ic));

  if(sym && IS_DECL(sym->type))
    {
      // Find size of return value.
      specifier *spec;
      if(sym->type->select.d.dcl_type != FUNCTION)
        NOTUSEDERROR();
      spec = &(sym->etype->select.s);
      if(spec->noun == V_VOID)
        size = 0;
      else if(spec->noun == V_CHAR || spec->noun == V_BOOL || spec->noun == V_BITINT && SPEC_BITINTWIDTH(sym->etype) <= 8)
        size = 1;
      else if(spec->noun == V_INT && !(spec->b_long) || spec->noun == V_BITINT && SPEC_BITINTWIDTH(sym->etype) <= 16)
        size = 2;
      else
        size = 4;

      // Check for returned pointer.
      sym_lnk = sym->type;
      while (sym_lnk && !IS_PTR (sym_lnk))
        sym_lnk = sym_lnk->next;
      if(IS_PTR(sym_lnk))
        size = 2;
    }
  else
    {
      NOTUSEDERROR();
      return TRUE;
    }

  switch(*what)
    {
    case 'a':
      return(size == 1 || size == 2);
    case 'p':
      return(size == 2);
    default:
      return false;
    }
}

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
    if (isspace(*p) || *p == ',')
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
static bool argIs(const char *arg, const char *what)
{
  if (arg == NULL || strlen (arg) == 0)
    return false;

  while (isblank ((unsigned char)(arg[0])))
    arg++;

  if (arg[0] == ',')
    arg++;

  while (isblank ((unsigned char)(arg[0])))
    arg++;

  return !strncmp(arg, what, strlen(what)) &&
    (!arg[strlen(what)] || isspace((unsigned char)(arg[strlen(what)])) || arg[strlen(what)] == ',' || arg[strlen(what)] == '.');
}

static bool
pdkMightReadFlag(const lineNode *pl, const char *what)
{
  if (lineIsInst (pl, "push") && argIs (pl->line + 4, "af") || lineIsInst (pl, "pushaf"))
    return true;

  if (lineIsInst (pl, "t0sn.io") || lineIsInst (pl, "t1sn.io"))
    return argIs(strchr (pl->line, ','), what);

  if(lineIsInst (pl, "addc") ||
    lineIsInst (pl, "subc"))
    return !strcmp(what, "c");

  return false;
}

static bool
pdkMightRead (const lineNode *pl, const char *what)
{
//printf("pdkMightRead() for |%s|%s|\n", pl->line, what);
  if (!strcmp(what, "z") || !strcmp(what, "c") || !strcmp(what, "ac") || !strcmp(what, "ov"))
    return (pdkMightReadFlag(pl, what));
  else if (strcmp(what, "a") && strcmp(what, "p"))
    return true;

  // Instructions that never read anything.
  if (lineIsInst (pl, "clear") ||
    lineIsInst (pl, "engint") ||
    lineIsInst (pl, "disgint") ||
    lineIsInst (pl, "goto") ||
    lineIsInst (pl, "nop") ||
    lineIsInst (pl, "pop") || lineIsInst (pl, "popaf") ||
    lineIsInst (pl, "set0.io") || lineIsInst (pl, "set1.io") ||
    lineIsInst (pl, "stopsys") ||
    lineIsInst (pl, "t0sn.io") || lineIsInst (pl, "t1sn.io") || // They read I/O (maybe flags) but neither a nor p.
    lineIsInst (pl, "wdreset"))
    return false;

  if (lineIsInst (pl, "ret") && strchr (pl->line + 2, '#') && !strcmp (what, "a"))
    return false;
  if (lineIsInst (pl, "ret"))
    return isReturned(what);

  if (lineIsInst (pl, "mov") ||
    lineIsInst (pl, "mov.io"))
    return argIs (strchr (pl->line, ','), what);

  if (lineIsInst (pl, "push") && argIs (pl->line + 4, "af") || lineIsInst (pl, "pushaf"))
    return !strcmp(what, "a");

  // Two-operand instructions that read both operands
  if (lineIsInst (pl, "add") ||
    lineIsInst (pl, "and") ||
    lineIsInst (pl, "or") ||
    lineIsInst (pl, "sub") ||
    lineIsInst (pl, "xch") ||
    lineIsInst (pl, "xor"))
    return argIs (pl->line + 3, what) || argIs (strchr (pl->line, ','), what);
  if (lineIsInst (pl, "idxm"))
    return argIs (pl->line + 4, what) || argIs (strchr (pl->line, ','), what);
  if (lineIsInst (pl, "ceqsn") ||
    lineIsInst (pl, "cneqsn") ||
    lineIsInst (pl, "xor.io"))
    return argIs (pl->line + 6, what) || argIs (strchr (pl->line, ','), what);

  // One-operand instructions
  if (lineIsInst (pl, "dec") ||
    lineIsInst (pl, "inc") ||
    lineIsInst (pl, "neg") ||
    lineIsInst (pl, "not") ||
    lineIsInst (pl, "sl") ||
    lineIsInst (pl, "slc") ||
    lineIsInst (pl, "sr") ||
    lineIsInst (pl, "src"))
    return argIs (pl->line + 3, what);
  if (lineIsInst (pl, "addc") && !strchr (pl->line + 4, ',') || // addc a / addc M
    lineIsInst (pl, "set0") || lineIsInst (pl, "set1") || // Technically two-operand, but second operand is a literal
    lineIsInst (pl, "subc") && !strchr (pl->line + 4, ',') || // subc a / subc M)
    lineIsInst (pl, "swap"))
    return argIs (pl->line + 4, what);
  if (lineIsInst (pl, "dzsn") ||
    lineIsInst (pl, "izsn") ||
    lineIsInst (pl, "pcadd") ||
    lineIsInst (pl, "stt16"))
    return argIs (pl->line + 5, what);

  return true; // Fail-safe: we have no idea what happens at this line, so assume it might read anything.
}

static bool
pdkSurelyWritesFlag(const lineNode *pl, const char *what)
{
  if (lineIsInst (pl, "pop") && argIs (pl->line + 4, "af") || lineIsInst (pl, "popaf"))
    return true;

  // Instructions that write all flags
  if (lineIsInst (pl, "add") ||
    lineIsInst (pl, "addc") ||
    lineIsInst (pl, "ceqsn") ||
    lineIsInst (pl, "cneqsn") ||
    lineIsInst (pl, "dec") ||
    lineIsInst (pl, "dzsn") ||
    lineIsInst (pl, "inc") ||
    lineIsInst (pl, "izsn") ||
    lineIsInst (pl, "sub") ||
    lineIsInst (pl, "subc"))
    return true;

  // Instructions that write c only
  if (lineIsInst (pl, "sl") ||
    lineIsInst (pl, "slc") ||
    lineIsInst (pl, "sr") ||
    lineIsInst (pl, "src") ||
    lineIsInst (pl, "swapc.io"))
    return !strcmp(what, "c");

  // Instructions that write z only
  if (lineIsInst (pl, "and") ||
    lineIsInst (pl, "neg") ||
    lineIsInst (pl, "not") ||
    lineIsInst (pl, "or") ||
    lineIsInst (pl, "xor") ||
    lineIsInst (pl, "xor.io"))
    return !strcmp(what, "z");

  // mov writes z iff the destination is a and the source not an immediate.
  if (!strcmp(what, "z") && !strchr(pl->line, '#'))
    {
      if ( (lineIsInst (pl, "mov") && pl->line[4] == 'a' && pl->line[5] == ',') ||
         (lineIsInst (pl, "mov.io") && pl->line[7] == 'a' && pl->line[8] == ','))
        return true;
    }

  if (lineIsInst (pl, "mov") || lineIsInst (pl, "mov.io") ||
    lineIsInst (pl, "clear") ||
    lineIsInst (pl, "goto") ||
    lineIsInst (pl, "t0sn") || lineIsInst (pl, "t0sn.io") ||
    lineIsInst (pl, "t1sn") || lineIsInst (pl, "t1sn.io") ||
    lineIsInst (pl, "xch") ||
    lineIsInst (pl, "swap") ||
    lineIsInst (pl, "idxm"))
    return false;

  // By calling convention, caller has to save flags.
  if (lineIsInst (pl, "ret") || lineIsInst (pl, "call"))
    return true;

  return false; // Fail-safe: we have no idea what happens at this line, so assume it writes nothing.
}

static bool
pdkSurelyWrites(const lineNode *pl, const char *what)
{
  if (!strcmp(what, "z") || !strcmp(what, "c") || !strcmp(what, "ac") || !strcmp(what, "ov"))
    return (pdkSurelyWritesFlag(pl, what));

  if (lineIsInst (pl, "mov") || lineIsInst (pl, "idxm") ||
    lineIsInst (pl, "set1") ||
    lineIsInst (pl, "set0"))
    return argIs (pl->line + 4, what);
  if (lineIsInst (pl, "mov.io") ||
    lineIsInst (pl, "set1.io") ||
    lineIsInst (pl, "set0.io"))
    return argIs (pl->line + 7, what);
  if (lineIsInst (pl, "swapc.io"))
    return argIs (pl->line + 9, what);

  if (lineIsInst (pl, "pop") && argIs (pl->line + 4, "af") || lineIsInst (pl, "popaf"))
    return !strcmp (what, "a");

  // Instructions that never write anything
  if (lineIsInst (pl, "ceqsn") || lineIsInst (pl, "cneqsn") || // Might write flags, but nothing else.
    lineIsInst (pl, "goto") ||
    lineIsInst (pl, "t0sn") || lineIsInst (pl, "t0sn.io") ||
    lineIsInst (pl, "t1sn") || lineIsInst (pl, "t1sn.io"))
    return false;

  // One-operand instructions that write their argument
  if (lineIsInst (pl, "neg") ||
    lineIsInst (pl, "not") ||
    lineIsInst (pl, "inc") ||
    lineIsInst (pl, "dec") ||
    lineIsInst (pl, "xch") ||
    lineIsInst (pl, "not") ||
    lineIsInst (pl, "sl") ||
    lineIsInst (pl, "slc") ||
    lineIsInst (pl, "sr") ||
    lineIsInst (pl, "src"))
    return argIs (pl->line + 3, what);
  if (lineIsInst (pl, "dzsn") ||
    lineIsInst (pl, "clear") ||
    lineIsInst (pl, "izsn") ||
    lineIsInst (pl, "ldt16"))
    return argIs (pl->line + 5, what);

  return false;
}


static bool
pdkUncondJump(const lineNode *pl)
{
  return (lineIsInst (pl, "goto") || lineIsInst (pl, "pcadd"));
}

static bool
pdkCondJump(const lineNode *pl)
{
  return (lineIsInst (pl, "ceqsn") || lineIsInst (pl, "cneqsn") ||
    lineIsInst (pl, "t0sn") || lineIsInst (pl, "t1sn") ||
    lineIsInst (pl, "t0sn.io") || lineIsInst (pl, "t1sn.io") ||
    lineIsInst (pl, "izsn") || lineIsInst (pl, "dzsn"));
}

static bool
pdkSurelyReturns(const lineNode *pl)
{
  return(lineIsInst (pl, "ret") || lineIsInst (pl, "reti") );
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

      if(pdkMightRead(*pl, what))
        {
          D(("S4O_RD_OP\n"));
          return S4O_RD_OP;
        }

      // Check writes before conditional jumps, some jumps (btjf, btjt) write 'c'
      if(pdkSurelyWrites(*pl, what))
        {
          D(("S4O_WR_OP\n"));
          return S4O_WR_OP;
        }

      if(pdkUncondJump(*pl))
        {
          *pl = findLabel (*pl);
            if (!*pl)
              {
                D(("S4O_ABORT at unconditional jump\n"));
                return S4O_ABORT;
              }
        }
      if(pdkCondJump(*pl))
        {
          *plCond = (*pl)->next->next;
          if (!*plCond)
            {
              D(("S4O_ABORT at conditional jump\n"));
              return S4O_ABORT;
            }
          D(("S4O_CONDJMP\n"));
          return S4O_CONDJMP;
        }

      /* Don't need to check for de, hl since pdkMightRead() does that */
      if(pdkSurelyReturns(*pl))
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

bool pdknotUsed(const char *what, lineNode *endPl, lineNode *head)
{
  lineNode *pl;

  _G.head = head;

  unvisitLines (_G.head);

  pl = endPl->next;
  return (doTermScan (&pl, what));
}

bool pdknotUsedFrom(const char *what, const char *label, lineNode *head)
{
  lineNode *cpl;

  for (cpl = head; cpl; cpl = cpl->next)
    if (cpl->isLabel && !strncmp (label, cpl->line, strlen(label)))
      return (pdknotUsed (what, cpl, head));

  return false;
}

int
pdkinstructionSize(lineNode *pl)
{
  return 1;
}

