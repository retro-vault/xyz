/**
 * Debug symbol emission logic for CDB format.
 *
 * This module provides functions for generating debug information files
 * in the CDB format, including symbol tables, function scopes, type
 * declarations, and line information for use in debugging tools.
 *
 * Copyright (C) 2025 [Your Name or Org]
 * Released under the GNU General Public License v2 or later.
 */

#pragma once

#include <stdio.h>   /* for FILE */
#include <stdarg.h>  /* for va_list */
#include <stdbool.h> /* for bool */

struct symbol;
struct iCode;
struct structdef;
struct sym_link;
struct reg_info;

/* Type representing a debug file emitter */
typedef struct
{
    int (*openFile)(const char *file);
    int (*closeFile)(void);
    int (*writeModule)(const char *name);
    int (*writeFunction)(struct symbol *pSym, struct iCode *ic);
    int (*writeEndFunction)(struct symbol *pSym, struct iCode *ic, int offset);
    int (*writeLabel)(struct symbol *pSym, const struct iCode *ic);
    int (*writeScope)(struct iCode *ic);
    int (*writeSymbol)(struct symbol *pSym);
    int (*writeType)(struct structdef *sdef, int block, int inStruct, const char *tag);
    int (*writeCLine)(struct iCode *ic);
    int (*writeALine)(const char *module, int line);
    int (*writeFrameAddress)(const char *variable, struct reg_info *reg, int offset);
} DEBUGFILE;

extern DEBUGFILE cdbDebugFile;
extern FILE *cdbFilePtr;
extern const char *cdbModuleName;

/* Exported functions */
int cdbOpenFile(const char *file);
int cdbCloseFile(void);
int cdbWriteFunction(struct symbol *pSym, struct iCode *ic);
int cdbWriteEndFunction(struct symbol *pSym, struct iCode *ic, int offset);
int cdbWriteLabel(struct symbol *pSym, const struct iCode *ic);
int cdbWriteScope(struct iCode *ic);
int cdbWriteSymbol(struct symbol *pSym);
int cdbWriteType(struct structdef *sdef, int block, int inStruct, const char *tag);
int cdbWriteModule(const char *name);
int cdbWriteCLine(struct iCode *ic);
int cdbWriteALine(const char *module, int line);
int cdbWriteFrameAddress(const char *variable, struct reg_info *reg, int offset);
int cdbWriteBasicSymbol(struct symbol *sym, int isStructSym, int isFunc);
void cdbTypeInfo(struct sym_link *type);
