/*-------------------------------------------------------------------------
  SDCCpeeph.h - Header file for The peep hole optimizer: for interpreting 
                the peep hole rules

  Copyright (C) 1999, Sandeep Dutta . sandeep.dutta@usa.net

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

#ifndef  SDCCPEEPH_H
#define SDCCPEEPH_H 1

#include <ctype.h>

#include "SDCCgen.h"

#define MAX_PATTERN_LEN 256

typedef struct peepRule
  {
    lineNode *match;
    lineNode *replace;
    unsigned int restart:1;
    unsigned int barrier:1;
    char *cond;
    hTab *vars;
    struct peepRule *next;
  }
peepRule;

typedef struct
  {
    char name[SDCC_NAME_MAX + 1];
    int refCount;
    /* needed for deadMove: */
    bool passedLabel;
    int jmpToCount;
  }
labelHashEntry;

bool isLabelDefinition (const char *line, const char **start, int *len,
                        bool isPeepRule);

extern hTab *labelHash;
labelHashEntry *getLabelRef (const char *label, lineNode *head);

void initPeepHole (void);
void peepHole (lineNode **);

const char * StrStr (const char * str1, const char * str2);

// Check if asm line pl is instruction inst (i.e. the first non-whitespace is inst).
static inline bool lineIsInst (const lineNode *pl, const char *inst)
{
  return (!STRNCASECMP(pl->line, inst, strlen(inst)) && (!pl->line[strlen(inst)] || isspace((unsigned char)(pl->line[strlen(inst)]))));
}

// Get a pointer pointing just beyond the argument starting at arg (in an asxxxx syntax line).
static inline const char *argEnd (const char *arg)
{
  if (!arg)
    return (NULL);

  for(unsigned int parens = 0; *arg; arg++)
    {
      if (*arg == '(' || *arg == '[')
        parens++;
      if (*arg == ')' || *arg == ']')
        parens--;
      if ((isspace (*arg) || *arg == ',') && !parens)
        break;
    }

  return (arg);
}

// Get a pointer pointing to the start of argument i (in an asxxxx syntax line, numbered starting at 0).
static inline const char *lineArg (const lineNode *pl, int i)
{
  const char *arg = i ? argEnd (lineArg (pl, i - 1)) : pl->line;

  if (!arg)
    return (NULL);

  // Skip instruction menmonic (for argument 0) or the comma (for further arguments), and any whitespace beside it.
  while (isspace (*arg))
    arg++;
  while (*arg && !isspace (*arg) && *arg != ',')
    arg++;
  while (isspace (*arg) || *arg == ',')
    arg++;

  // Check for comment
  if (*arg == ';')
    return (NULL);

  if (*arg == 0 || *arg == '\n')
    return (NULL);

  return (arg);
}

#endif

