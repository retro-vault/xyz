/**
 * Generic linked list (set) structure and manipulation API.
 *
 * This module defines a simple singly-linked list type called `set`
 * and a suite of functions for managing and processing these lists.
 * It is designed to store arbitrary pointers (`void*`) and is used
 * throughout the compiler for tracking symbols, options, segments, etc.
 *
 * Includes variadic match, filter, and merge functions with function
 * pointer hooks and conditional destructors.
 *
 * Copyright (C) 1998, Sandeep Dutta <sandeep.dutta@usa.net>
 * Released under the GNU General Public License v2 or later.
 */

#pragma once

#include <stdarg.h>

/* General-purpose exception annotations (optional use) */
#ifndef THROWS
#define THROWS
#define THROW_NONE 0
#define THROW_SRC 1
#define THROW_DEST 2
#define THROW_BOTH 3
#endif

/* Macros for variadic set functions */
#define DEFSETFUNC(fname) int fname(void *item, va_list ap)
#define V_ARG(type, var) type var = va_arg(ap, type)

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct set
  {
    void *item;
    struct set *curr;
    struct set *next;
  } set;

  set *newSet(void);
  void *addSet(set **, void *);
  void *addSetHead(set **, void *);
  void *getSet(set **);
  void deleteSetItem(set **, void *);
  void replaceSetItem(set *, void *olditem, void *newitem);
  void deleteItemIf(set **, int (*cond)(void *, va_list), ...);
  void destructItemIf(set **, void (*destructor)(void *), int (*cond)(void *, va_list), ...);
  int isinSet(const set *, const void *);
  typedef int (*insetwithFunc)(void *, void *);
  int isinSetWith(set *, void *, insetwithFunc);
  int applyToSet(set *list, int (*somefunc)(void *, va_list), ...);
  int applyToSetFTrue(set *list, int (*somefunc)(void *, va_list), ...);
  void mergeSets(set **sset, set *list);
  set *unionSets(set *, set *, int);
  set *unionSetsWith(set *, set *, int (*cFunc)(), int);
  set *intersectSets(set *, set *, int);
  void *addSetIfnotP(set **, void *);
  set *setFromSet(const set *);
  set *setFromSetNonRev(const set *);
  int isSetsEqual(const set *, const set *);
  set *subtractFromSet(set *, set *, int);
  int elementsInSet(const set *);
  void *indexSet(set *, int);
  set *intersectSetsWith(set *, set *, int (*cFunc)(void *, void *), int);
  int isSetsEqualWith(set *, set *, int (*cFunc)(void *, void *));
  void *peekSet(const set *);
  void *setFirstItem(set *);
  void *setNextItem(set *);
  void setToNull(void **);
  set *reverseSet(set *);
  void deleteSet(set **s);

#ifdef __cplusplus
}
#endif
