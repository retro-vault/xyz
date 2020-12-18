/*
 *  types.h
 *  common types used in code (do not include stdint.h!)
 *
 *  tomaz stih may 25 2012
 */
#ifndef _TYPES_H
#define _TYPES_H

#define NULL    ( (word)0 )
#define TRUE    ( (byte)1 )
#define FALSE   ( (byte)0 )

/* usual */
typedef signed char         byte;
typedef byte                boolean;
typedef byte                result;
typedef unsigned short int  word;
typedef word                handle;
typedef byte*               string;

/* general purpose macros */
#define MAX(a,b) ( (a>b?a:b) )
#define MIN(a,b) ( (a<b?a:b) )

#endif