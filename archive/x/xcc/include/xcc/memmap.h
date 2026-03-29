/**
 * Memory mapping and allocation logic for compiler segments.
 *
 * This module defines the `memmap` and `namedspacemap` structures for representing
 * memory segments such as code, data, xdata, stack, overlay, etc., and provides
 * allocation and symbol management across them.
 *
 * Used during backend allocation passes to place global/local variables and overlay segments
 * into their correct target-specific address space.
 *
 * Copyright (C) 2011 Philipp Klaus Krause <philipp@informatik.uni-frankfurt.de>
 * Released under the GNU General Public License v2 or later.
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>

#include <xcc/dbuf.h>

/* Forward declarations */
struct set;
struct value;
struct eBBlock;

/* Memory map segment representation */
typedef struct memmap
{
    unsigned char pageno;
    const char *sname;
    char dbName;
    int ptrType;
    int slbl;
    unsigned sloc;
    unsigned fmap : 1;
    unsigned paged : 1;
    unsigned direct : 1;
    unsigned bitsp : 1;
    unsigned codesp : 1;
    unsigned regsp : 1;
    struct dbuf_s oBuf;
    struct set *syms;
} memmap;

/* Named memory space mapping */
typedef struct namedspacemap
{
    char *name;
    bool is_const;
    memmap *map;
    struct namedspacemap *next;
} namedspacemap;

/* Segment name macros */
#define XSTACK_NAME port->mem.xstack_name
#define ISTACK_NAME port->mem.istack_name
#define CODE_NAME port->mem.code_name
#define DATA_NAME port->mem.data_name
#define INITIALIZED_NAME port->mem.initialized_name
#define INITIALIZER_NAME port->mem.initializer_name
#define IDATA_NAME port->mem.idata_name
#define PDATA_NAME port->mem.pdata_name
#define XDATA_NAME port->mem.xdata_name
#define XIDATA_NAME port->mem.xidata_name
#define XINIT_NAME port->mem.xinit_name
#define BIT_NAME port->mem.bit_name
#define REG_NAME port->mem.reg_name
#define STATIC_NAME port->mem.static_name
#define GSFINAL_NAME port->mem.post_static_name
#define HOME_NAME port->mem.home_name
#define OVERLAY_NAME port->mem.overlay_name
#define CONST_NAME port->mem.const_name
#define CABS_NAME port->mem.cabs_name
#define XABS_NAME port->mem.xabs_name
#define IABS_NAME port->mem.iabs_name

/* Memory segment property macros */
#define IN_BITSPACE(map) (map && map->bitsp)
#define IN_STACK(x) (IS_SPEC(x) && (SPEC_OCLS(x) == xstack || SPEC_OCLS(x) == istack))
#define IN_FARSPACE(map) (map && map->fmap)
#define IN_DIRSPACE(map) (map && map->direct)
#define IN_PAGEDSPACE(map) (map && map->paged)
#define IN_CODESPACE(map) (map && map->codesp)
#define IN_REGSP(map) (map && map->regsp)
#define PTR_TYPE(map) (map ? (map->ptrType ? map->ptrType : POINTER) : port->unqualified_pointer)

#ifdef __cplusplus
extern "C"
{
#endif

    /* Globals */
    extern FILE *junkFile;

    extern memmap *xstack;
    extern memmap *istack;
    extern memmap *code;
    extern memmap *data;
    extern memmap *initialized;
    extern memmap *initializer;
    extern memmap *pdata;
    extern memmap *xdata;
    extern memmap *xidata;
    extern memmap *xinit;
    extern memmap *idata;
    extern memmap *bit;
    extern memmap *statsg;
    extern memmap *c_abs;
    extern memmap *x_abs;
    extern memmap *i_abs;
    extern memmap *d_abs;
    extern memmap *sfr;
    extern memmap *sfrbit;
    extern memmap *reg;
    extern memmap *generic;
    extern memmap *overlay;
    extern memmap *eeprom;
    extern memmap *home;

    extern namedspacemap *namedspacemaps;
    extern int fatalError;
    extern struct set *ovrSetSets;

    /* Functions */
    memmap *allocMap(char, char, char, char, char, char, unsigned, const char *, char, int);
    void initMem(void);
    bool defaultOClass(struct symbol *);
    void allocGlobal(struct symbol *);
    void allocLocal(struct symbol *);
    void allocParms(struct value *, bool smallc);
    void deallocParms(struct value *);
    void deallocLocal(struct symbol *);
    int allocVariables(struct symbol *);
    void overlay2Set(void);
    void overlay2data(void);
    void clearStackOffsets(void);
    void redoStackOffsets(void);
    void printAllocInfo(struct symbol *, struct dbuf_s *);
    void doOverlays(struct eBBlock **, int count);
    void deleteFromSeg(struct symbol *);

#ifdef __cplusplus
}
#endif
