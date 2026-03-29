/*-------------------------------------------------------------------------
  SDCCglobl.h - global macros etc required by all files

  Copyright (C) 1998, Sandeep Dutta . sandeep.dutta@usa.net

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

#ifndef SDCCGLOBL_H
#define SDCCGLOBL_H

#include <memory.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h> /* PATH_MAX? */
#if !defined(PATH_MAX) || (PATH_MAX < 2048)
#undef PATH_MAX
#define PATH_MAX 2048 /* define a reasonable value */
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#elif defined(_WIN32) && !defined(__MINGW32__)
#include "sdcc_vc.h"
#else
#include "sdccconf.h"
#endif

#include <xcc/set.h>
#include <xcc/err.h>

/*
 * Define host port dependant constants etc.
 */

#define UNIX_DIR_SEPARATOR_CHAR '/'
#define STRCASECMP strcasecmp
#define STRNCASECMP strncasecmp

#define IS_DIR_SEPARATOR(c) ((c) == DIR_SEPARATOR_CHAR)
#define IS_ABSOLUTE_PATH(f) (IS_DIR_SEPARATOR((f)[0]))
#define FILENAME_CMP(s1, s2) strcmp(s1, s2)

#define SPACE ' '

#define MAX_REG_PARMS 1

/* C++ doesn't like min and max macros */
#ifndef __cplusplus
#ifndef max
#define max(a, b) (a > b ? a : b)
#endif
#ifndef min
#define min(a, b) (a < b ? a : b)
#endif
#endif /* __cplusplus */

#ifndef THROWS
#define THROWS
#define THROW_NONE 0
#define THROW_SRC 1
#define THROW_DEST 2
#define THROW_BOTH 3
#endif

/* sizes in bytes  */
#define BOOLSIZE port->s.char_size
#define CHARSIZE port->s.char_size
#define SHORTSIZE port->s.short_size
#define INTSIZE port->s.int_size
#define LONGSIZE port->s.long_size
#define LONGLONGSIZE port->s.longlong_size
#define NEARPTRSIZE port->s.near_ptr_size
#define FARPTRSIZE port->s.far_ptr_size
#define GPTRSIZE port->s.ptr_size
#define FUNCPTRSIZE port->s.funcptr_size
#define BFUNCPTRSIZE port->s.banked_funcptr_size
#define BITSIZE port->s.bit_size
#define FLOATSIZE port->s.float_size

#define INITIAL_INLINEASM (4 * 1024)
#define DEFPOOLSTACK(type, size) \
  type *type##Pool;              \
  type *type##FreeStack[size];   \
  int type##StackPtr = 0;

#define PUSH(x, y) x##FreeStack[x##StackPtr++] = y
#define PEEK(x) x##FreeStack[x##StackPtr - 1]
#define POP(type) type##FreeStack[--type##StackPtr]
/* #define POP(x)    (x##StackPtr ? x##FreeStack[--x##StackPtr] :       \
   (assert(x##StackPtr),0)) */
#ifdef UNIX
#define EMPTY(x) (x##StackPtr <= 1 ? 1 : 0)
#else
#define EMPTY(x) (x##StackPtr == 0 ? 1 : 0)
#endif

#define COPYTYPE(start, end, from) (end = getSpec(start = from))

/* general purpose stack related macros */
#define STACK_DCL(stack, type, size) \
  typedef type t_##stack;            \
  t_##stack stack[size];             \
  t_##stack(*p_##stack) = stack - 1;

#define STACK_EMPTY(stack) ((p_##stack) < stack)
#define STACK_FULL(stack) ((p_##stack) >= (stack + \
                                           sizeof(stack) / sizeof(*stack) - 1))

#define STACK_PUSH_(stack, x) (*++p_##stack = (x))
#define STACK_POP_(stack) (*p_##stack--)

#define STACK_PUSH(stack, x) (STACK_FULL(stack)                       \
                                  ? (STACK_ERR(1, stack), *p_##stack) \
                                  : STACK_PUSH_(stack, x))

#define STACK_POP(stack) (STACK_EMPTY(stack)                   \
                              ? (STACK_ERR(-1, stack), *stack) \
                              : STACK_POP_(stack))

#define STACK_PEEK(stack) (STACK_EMPTY(stack)                  \
                               ? (STACK_ERR(0, stack), *stack) \
                               : *p_##stack)

#define STACK_ERR(o, stack) (fatal(1, E_STACK_VIOLATION, #stack, \
                                   (o < 0)                       \
                                       ? "underflow"             \
                                   : (o > 0)                     \
                                       ? "overflow"              \
                                       : "empty"))

/* for semantically partitioned nest level values */
#define LEVEL_UNIT 10000
#define SUBLEVEL_UNIT 1

/* optimization options */
struct optimize
{
  int global_cse;
  int ptrArithmetic;
  int label1;
  int label2;
  int label3;
  int label4;
  int loopInvariant;
  int loopInduction;
  int noLoopReverse;
  int codeSpeed;
  int codeSize;
  int lospre;
  int genconstprop;
  int allow_unsafe_read;
  int noStdLibCall;
};

/* overlay segment name and the functions
   that belong to it. used by pragma overlay */
typedef struct
{
  char *osname;     /* overlay segment name */
  int nfuncs;       /* number of functions in this overlay */
  char *funcs[128]; /* function name that belong to this */
} olay;

/* forward definition for variables accessed globally */
extern char *yytext;
extern char *lexFilename;           /* lex idea of current file name */
extern int lexLineno;               /* lex idea of line number of the current file */
extern const char *fullSrcFileName; /* full name for the source file; */
                                    /* can be NULL while linking without compiling */
extern const char *fullDstFileName; /* full name for the output file; */
                                    /* only given by -o, otherwise NULL */
extern const char *dstFileName;     /* destination file name without extension */
extern const char *moduleName;      /* module name is source file without path and extension */
                                    /* can be NULL while linking without compiling */
extern int seqPointNo;              /* current sequence point */
extern FILE *yyin;                  /* */
extern FILE *asmFile;               /* assembly output file */
extern FILE *cdbFile;               /* debugger symbol file */
extern long NestLevel;              /* NestLevel                 SDCC.y */
extern int stackPtr;                /* stack pointer             SDCC.y */
extern int xstackPtr;               /* external stack pointer    SDCC.y */
extern int reentrant;               /* /X flag has been sent     SDCC.y */
extern char buffer[PATH_MAX * 2];   /* general buffer           SDCCmain.c */
extern int currRegBank;             /* register bank being used  SDCCgens.c */
extern int RegBankUsed[4];          /* JCF: register banks used  SDCCmain.c */
extern int BitBankUsed;             /* MB: overlayable bit bank  SDCCmain.c */
extern struct symbol *currFunc;     /* current function    SDCCgens.c */
extern long cNestLevel;             /* block nest level  SDCCval.c */
extern int blockNo;                 /* maximum sequential block number */
extern int currBlockno;             /* sequential block number */
extern struct optimize optimize;
extern struct options options;
extern unsigned maxInterrupts;
extern int ignoreTypedefType;

/* Visible from SDCCmain.c */
extern set *preArgvSet;
extern set *relFilesSet;
extern set *libFilesSet;
extern set *libPathsSet;
extern set *libDirsSet; /* list of lib search directories */

void setParseWithComma(set **, const char *);

enum
{
  DUMP_RAW0 = 1,
  DUMP_RAW1,
  DUMP_CSE,
  DUMP_DFLOW,
  DUMP_GCSE,
  DUMP_DEADCODE,
  DUMP_LOOP,
  DUMP_LOOPG,
  DUMP_LOOPD,
  DUMP_LOSPRE,
  DUMP_GENCONSTPROP,
  DUMP_RANGE,
  DUMP_PACK,
  DUMP_RASSGN,
  DUMP_LRANGE,
  DUMP_CUSTOM0, // For temporary dump points
  DUMP_CUSTOM1  // For temporary dump points
};

struct _dumpFiles
{
  int id;
  char *ext;
  FILE *filePtr;
};

extern struct _dumpFiles dumpFiles[];

/* Define well known filenos if the system does not define them.  */
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#endif
