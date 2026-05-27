/* asxxxx.h */

/*
 *  Copyright (C) 1989-2025  Alan R. Baldwin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Alan R. Baldwin
 * 721 Berkeley St.
 * Kent, Ohio  44240
 *
 *   With enhancements from
 *
 *	John L. Hartman	(JLH)
 *	jhartman at compuserve dot com
 *
 *	Bill McKinnon (BM)
 *	w_mckinnon at conknet dot com
 *
 *	Boisy G. Petri (BGP)
 *	boisy at boisypitre dot com
 *
 *	Mike McCarty
 *	mike dot mccarty at sbcglobal dot net
 *
 *	Nick Downing
 *	downing dot nick at gmail dot com
 *
 *  3-Feb-00 KV:
 *      - add DS80C390 flat mode support.
 * 10-Nov-07 borutr:
 *      - change a_id from [NCPS] to pointer
 * 02-Feb-22 basxto/bbbbbr:
 *      - raise NCPS to 256 like in upstream
 */
 
#define VOID    void
#define OTHERSYSTEM

/*
 * System Include Files
 */

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <time.h>

/*
 * Local Definitions
 */

#define	VERSION	"V05.50.4+NoICE+SDCCmods-WIP-R14"
#define	COPYRIGHT "2025"

/*
 * To include NoICE Debugging set non-zero
 */
#define	NOICE	1

/*
 * To include SDCC Debugging set non-zero
 */
#define	SDCDB	1


/*
 * The assembler requires certain variables to have
 * at least 32 bits to allow correct address processing.
 *
 * The type INT32 is defined so that compiler dependent
 * variable sizes may be specified in one place.
 *
 * LONGINT is defined when INT32 is 'long' to
 * select the 'l' forms for format strings
 * and constants.
 */

/* Turbo C++ 3.0 for DOS */
/* 'int' is 16-bits, 'long' is 32-bits */

#ifdef	__TURBOC__
#define		INT32	long
#define		LONGINT
#endif

/* Symantec C++ V6.x/V7.x for DOS (not DOSX) */
/* 'int' is 16-bits, 'long' is 32-bits */

#ifdef	__SC__
#define		INT32	long
#define		LONGINT
#endif

/* The DEFAULT is 'int' is 32 bits */
#ifndef	INT32
#define		INT32	int
#endif

#if !defined(__BORLANDC__) && !defined(_MSC_VER)
#include <unistd.h>
#endif

/*)Module	asxxxx.h
 *
 *	The module asxxxx.h contains the definitions for constants,
 *	structures, global variables, and ASxxxx functions
 *	contained in the ASxxxx.c files.  The functions and
 *	global variables from the machine dependent files are
 *	also defined.
 */

/*
 *	 compiler/operating system specific definitions
 */

/* File/extension seperator */

#define	FSEPX	'.'

/*
 * PATH_MAX
 */
#include <limits.h>
#ifndef PATH_MAX                                    /* POSIX, but not required */
#  if defined(_MSC_VER) || defined(__BORLANDC__)    /* Microsoft C or Borland C */
#    define PATH_MAX        _MAX_PATH
#  else
#    define PATH_MAX        FILENAME_MAX            /* define a reasonable value */
#  endif
#endif

#ifdef _WIN32           /* WIN32 native */
#  define NATIVE_WIN32          1
#  ifdef __MINGW32__    /* GCC MINGW32 depends on configure */
#    include "../../sdccconf.h"
#  else
#    include "../../sdcc_vc.h"
#    define PATH_MAX    _MAX_PATH
#  endif
#else                   /* Assume *nix style system */
#  include "../../sdccconf.h"
#endif

/*
 * Error definitions
 */
#define	ER_NONE		0	/* No error */
#define	ER_WARNING	1	/* Warning */
#define	ER_ERROR	2	/* Assembly/Linker error */
#define	ER_FATAL	3	/* Fatal error */

/*
 * Assembler definitions.
 */
#define	LFTERM	'('		/* Left expression delimeter */
#define	RTTERM	')'		/* Right expression delimeter */

#define NCPS	256		/* Characters per symbol */
#define ASXHUGE     1000        /* A huge number */
#define NERR	3		/* Errors per line */
#define NINPUT	1024		/* Input buffer size */
#define NCODE	380		/* Listing code buffer size */
#define NTITL	80		/* Title buffer size */
#define	NSBTL	80		/* SubTitle buffer size */
#define	NHASH	(1 << 6)	/* Buckets in hash table */
#define	HMASK	(NHASH - 1)	/* Hash mask */
#define	NLPP	60		/* Lines per page */
#define	MAXFIL	6		/* Maximum command line input files */
#define	MAXINC	6		/* Maximum nesting of include files */
#define	MAXMCR	20		/* Maximum nesting of macro expansions */
#define	MAXIF	10		/* Maximum nesting of if/else/endif */
#define FILSPC      PATH_MAX    /* Chars. in filespec */

#define NLIST	0		/* No listing */
#define SLIST	1		/* Source only */
#define ALIST	2		/* Address only */
#define	BLIST	3		/* Address only with allocation */
#define CLIST	4		/* Code */
#define	ELIST	5		/* Equate or IF conditional evaluation */

#define	HLR_NLST	0x0080	/* For HLR file only */
#define	LIST_ERR	0x0001	/* Error Code(s) */
#define	LIST_LOC	0x0002	/* Location */
#define	LIST_BIN	0x0004	/* Generated Binary Value(s)*/
#define	LIST_EQT	0x0008	/* Assembler Equate Value */
#define	LIST_CYC	0x0010	/* Opcode Cycles */
#define	LIST_LIN	0x0020	/* Line Numbers */
#define	LIST_SRC	0x0040	/* Assembler Source Code */

#define	LIST_PAG	0x0080	/* Assembler Pagination */
#define	LIST_LST	0x0100	/* .LIST/.NLIST Listing */

#define	LIST_MD		0x0200	/* Macro Definition */
#define	LIST_ME		0x0400	/* Macro Expansion */
#define	LIST_MEB	0x0800	/* Macro Expansion Binary */
#define	LIST_MEL	0x1000	/* Macro Expansion Binary And Assembler Line */

#define	LIST_BITS	0x1FFF	/* LIST  Flags Mask */

#define	LIST_NONE	0x0000	/* NLIST Flags Mask */
#define	LIST_ASM	0x007F	/* LIST  Flags Mask for Assembler Line */
#define	LIST_NORM	0x03FF	/* LIST  Flags Mask */

#define	LIST_NOT	0x2000	/* Force Complement of Listing Mode */

#define	LIST_TORF	0x8000	/* IF-ENDIF Conditional Overide Flag */

#define T_ASM   0               /* Assembler Source File */
#define T_INCL  1               /* Assembler Include File */
#define T_MACRO 2               /* Assembler Macro */

/*
 * Opcode Cycle definitions (Must Be The Same In ASxxxx / ASLink)
 */
#define	CYCNT_BGN	'['	/* Cycle count begin delimiter */
#define	CYCNT_END	']'	/* Cycle count end   delimiter */

/*
 * OPCY_NONE bit set signifies no opcode cycles set.
 */
#define OPCY_NONE       ((char) 0x80)   /* Opcode Cycle Count Not Set */
#define OPCY_MASK       ((char) 0x7F)   /* Opcode Cycle Count MASK */

/*
 * NTXT must be defined to have the same value in
 * the ASxxxx assemblers and ASLink.
 *
 * The R Line coding allows only 4-bits for coding
 * the T Line index.  The MAXIMUM value for NTXT
 * is 16.  It should not be changed.
 */
#define	NTXT	16		/* Maximum T Line Values */
#define	NREL	16		/* Maximum R Line Values */

/*
 * Internal Definitions
 */
#define	dot	sym[0]		/* Dot, current loc */
#define	dca	area[0]		/* Dca, default code area */
#define dcb	bank[0]		/* Dcb, default code bank */

#define	hilo	sym[3].s_addr	/* hilo, byte order flag */
#define	mls	sym[4]		/* Mls, Macro local symbol */

/*
 *	The defined type 'a_uint' is used for all address and
 *	unsigned variable value calculations.  Its size is
 *	required to be at least 32-bits to allow upto
 *	32-bit addressing or 32-bit value manipulation.
 */
typedef	unsigned INT32 a_uint;

/*
 *	The defined type 'v_sint' is used for address and
 *	variable value calculations requiring a sign.
 *	Its size is required to be at least 32-bits to allow
 *	upto 32-bit addressing or 32-bit value manipulation.
 */
typedef	signed INT32 v_sint;

/*
 *	The area structure contains the parameter values for a
 *	specific program or data section.  The area structure
 *	is a linked list of areas.  The initial default area
 *      is "_CODE" defined in asdata.c, the next area structure
 *	will be linked to this structure through the structure
 *      element 'struct area *a_ap'.  The structure contains the
 *	area name, area reference number ("_CODE" is 0) determined
 *	by the order of .area directives, area size determined
 *	from the total code and/or data in an area, area fuzz is
 *	a variable used to track pass to pass changes in the
 *	area size caused by variable length instruction formats,
 *	and area flags which specify the area's relocation type.
 */
struct	area
{
	struct	area *a_ap;	/* Area link */
	char *	a_id;		/* Area Name */
	int	a_ref;		/* Ref. number */
	a_uint	a_size;		/* Area size */
	a_uint	a_fuzz;		/* Area fuzz */
	int	a_flag;		/* Area flags */
/* sdas specific */
        a_uint  a_addr;         /* Area address */
/* end sdas specific */
};

/*
 *	The "A_" area constants define values used in
 *	generating the assembler area output data.
 *
 * Area flags
 *
 *	   7     6     5     4     3     2     1     0
 *	+-----+-----+-----+-----+-----+-----+-----+-----+
 *      | BIT |XDATA|DATA | PAG | ABS | OVR |     |     |
 *	+-----+-----+-----+-----+-----+-----+-----+-----+
 */

#define A_CON   0000            /* Concatenating */
#define A_OVR   0004            /* Overlaying */
#define A_REL   0000            /* Relocatable */
#define A_ABS   0010            /* absolute */
#define A_NOPAG 0000            /* Non-Paged */
#define A_PAG   0020            /* Paged */

/* sdas specific */
/* Additional flags for 8051 address spaces */
#define A_DATA  0000            /* data space (default)*/
#define A_CODE  0040            /* code space */
#define A_XDATA 0100            /* external data space */
#define A_BIT   0200            /* bit addressable space */

#define A_NOLOAD  0400          /* nonloadable */
#define A_LOAD  0000            /* loadable (default) */
/* end sdas specific */

/*
 *	The "R_" relocation constants define values used in
 *	generating the assembler relocation output data for
 *	areas, symbols, and code.
 *
 * Relocation flags
 *
 *	   7     6     5     4     3     2     1     0
 *	+-----+-----+-----+-----+-----+-----+-----+-----+
 *      | MSB | PAGn| PAG0| USGN| BYT2| PCR | SYM | BYT |
 *	+-----+-----+-----+-----+-----+-----+-----+-----+
 */

#define R_BYTE  0x01            /*  8 bit */
#define R_WORD  0x00            /* 16 bit */

#define R_BYT1  0x00            /* Byte count for R_BYTE = 1 */
#define R_BYTX  0x08            /* Byte count for R_BYTE = 2 */
#define R_HIB   0x200           /* If R_BYTE & R_BYT3 are set, linker
                                 * will select byte 3 of the relocated
                                 * 24 bit address.
				 */

#define R_SGND  0x00            /* Signed Byte */
#define R_USGN  0x10            /* Unsigned Byte */

#define R_LSB   0x00            /* low byte */
#define R_MSB   0x80            /* high byte */

#define R_BIT   0x400           /* Linker will convert from byte-addressable
                                 * space to bit-addressable space.
				 */


#define R_AREA  0x00            /* Base type */
#define R_SYM   0x02

/*
 * Note:  The PAGE modes and PCR modes are mutually exclusive !!!
 *
 *
 * Paging Modes:
 */

#define	R_NOPAG	0x0000		/* Page Mode */
#define R_PAG0  0x0020          /* Page '0' */
#define R_PAGN  0x0040          /* Page 'nnn' */
#define R_PAGX  0x0060          /* Page 'x', Extended Relocation Mode */

/*
 * PCR Modes:
 */

#define R_PCR   0x04

#define R_J11           (R_WORD|R_BYTX)          /* JLH: 11 bit JMP and CALL (8051) */
#define R_J19           (R_WORD|R_BYTX|R_MSB)   /* 19 bit JMP/CALL (DS80C390)      */
#define R_C24           (R_WORD|R_BYT1|R_MSB)   /* 24 bit address (DS80C390)       */
#define R_J19_MASK      (R_BYTE|R_BYTX|R_MSB)

#define IS_R_J19(x)     (((x) & R_J19_MASK) == R_J19)
#define IS_R_J11(x)     (((x) & R_J19_MASK) == R_J11)
#define IS_C24(x)       (((x) & R_J19_MASK) == R_C24)

/*
 * Basic Relocation Modes
 */

#define	R_NORM	0x0000		/* No Bit Positioning */

/*
 * Extended Relocation Modes are defined in
 * the ___pst.c files.
 *
 *	#define	R_0100	0x0100	Extended mode 1
 *	...
 *	#define	R_0F00	0x0F00	Extended mode 15
 */

#define R_ESCAPE_MASK   0xf0    /* Used to escape relocation modes
                                 * greater than 0xff in the .rel
                                 * file.
				 */

/*
 * Listing Control Flags
 */

#define R_HIGH  0040000         /* High Byte */
#define R_BYT3  0x100           /* if R_BYTE is set, this is a
                                 * 3 byte address, of which
                                 * the linker must select one byte.
				 */
#define	R_RELOC	0100000		/* Relocation */

#define	R_DEF	00		/* Global def. */
#define	R_REF	01		/* Global ref. */
#define	R_REL	00		/* Relocatable */
#define	R_ABS	02		/* Absolute */
#define	R_GBL	00		/* Global */
#define	R_LCL	04		/* Local */

/*
 *	The mne structure is a linked list of the assembler
 *	mnemonics and directives.  The list of mnemonics and
 *	directives contained in the device dependent file
 *	xxxpst.c are hashed and linked into NHASH lists in
 *	module assym.c by syminit().  The structure contains
 *	the mnemonic/directive name, a subtype which directs
 *	the evaluation of this mnemonic/directive, a flag which
 *	is used to detect the end of the mnemonic/directive
 *	list in xxxpst.c, and a value which is normally
 *	associated with the assembler mnemonic base instruction
 *	value.
 */
struct	mne
{
	struct	mne *m_mp;	/* Hash link */
	char	*m_id;		/* Mnemonic (JLH) */
	char	m_type;		/* Mnemonic subtype */
	char	m_flag;		/* Mnemonic flags */
	a_uint	m_valu;		/* Value */
};

/*
 *	The sym structure is a linked list of symbols defined
 *	in the assembler source files.  The first symbol is "."
 *	defined in asdata.c.  The entry 'struct tsym *s_tsym'
 *	links any temporary symbols following this symbol and
 *	preceeding the next normal symbol.  The structure also
 *      contains the symbol's name, type (NEW or USER),
 *	flag (global, assigned, and multiply defined), a pointer
 *	to the area structure defining where the symbol is
 *	located, a reference number assigned by outgsd() in
 *	asout.c, and the symbols address relative to the base
 *	address of the area where the symbol is located.
 */
struct	sym
{
	struct	sym  *s_sp;	/* Hash link */
	struct	tsym *s_tsym;	/* Temporary symbol link */
	char	*s_id;		/* Symbol (JLH) */
	char	s_type;		/* Symbol subtype */
	char	s_flag;		/* Symbol flags */
	struct	area *s_area;	/* Area line, 0 if absolute */
	int	s_ref;		/* Ref. number */
	a_uint	s_addr;		/* Address */
/* sdas specific */
        a_uint  s_org;          /* Start Address if absolute */
/* end sdas specific */
};

#define S_EOL           040     /* End mark for ___pst files */

#define	S_NEW		0	/* New  Name (External) */
#define	S_USER		1	/* User Name (Assigned) */

#define	S_LCL		001	/* Local Variable */
#define	S_GBL		002	/* Global Variable */
#define	S_ASG		004	/* Assigned Value */
#define	S_MDF		010	/* Multiple Definition */

#define	S_OPTN		1	/* .enabl, .dsabl */
#define	  O_ENBL     1		/* .enabl */
#define	  O_DSBL     0		/* .dsabl */
#define	S_PAGE		2	/* .page */
#define	S_HEADER	3	/* .title, .sbttl */
#define	  O_TITLE    0		/* .title */
#define	  O_SBTTL    1		/* .sbttl */
#define	S_MODUL		4	/* .module */
#define	S_INCL		5	/* .include, .incbin */
#define	  I_CODE     0		/* .include */
#define	  I_BNRY     1		/* .incbin */
#define	S_AREA		6	/* .area */
#define S_ATYP          7       /* .area type */
#define	S_ORG		8	/* .org */
#define	S_RADIX		9	/* .radix */
#define	S_GLOBL		10	/* .globl */
#define	S_LOCAL		11	/* .local */
#define	S_CONDITIONAL	12	/* .if, .iif, .else, .endif, ... */
#define	  O_IF       0		/* .if */
#define	  O_IFF	     1		/* .iff */
#define	  O_IFT	     2		/* .ift */
#define	  O_IFTF     3		/* .iftf */
#define	  O_IFDEF    4		/* .ifdef */
#define	  O_IFNDEF   5		/* .ifndef */
#define	  O_IFGT     6		/* .ifgt (BGP) */
#define	  O_IFLT     7		/* .iflt (BGP) */
#define	  O_IFGE     8		/* .ifge (BGP) */
#define	  O_IFLE     9		/* .ifle (BGP) */
#define	  O_IFEQ     10		/* .ifeq (BGP) */
#define	  O_IFNE     11		/* .ifne (BGP) */
#define	  O_IFB      12		/* .ifb */
#define	  O_IFNB     13		/* .ifnb */
#define	  O_IFIDN    14		/* .ifidn */
#define	  O_IFDIF    15		/* .ifdif */
#define	  O_IFEND    20		/* end of .if conditionals */
#define	  O_IIF      20		/* .iif */
#define	  O_IIFF     21		/* .iiff */
#define	  O_IIFT     22		/* .iift */
#define	  O_IIFTF    23		/* .iiftf */
#define	  O_IIFDEF   24		/* .iifdef */
#define	  O_IIFNDEF  25		/* .iifndef */
#define	  O_IIFGT    26		/* .iifgt */
#define	  O_IIFLT    27		/* .iiflt */
#define	  O_IIFGE    28		/* .iifge */
#define	  O_IIFLE    29		/* .iifle */
#define	  O_IIFEQ    30		/* .iifeq */
#define	  O_IIFNE    31		/* .iifne */
#define	  O_IIFB     32		/* .iifb */
#define	  O_IIFNB    33		/* .iifnb */
#define	  O_IIFIDN   34		/* .iifidn */
#define	  O_IIFDIF   35		/* .iifdif */
#define	  O_IIFEND   40		/* end of .iif conditionals */
#define	  O_ELSE     40		/* .else */
#define	  O_ENDIF    41		/* .endif */
#define	S_LISTING	13	/* .nlist, .list */
#define	  O_LIST     0		/* .list */
#define	  O_NLIST    1		/* .nlist */
#define	S_EQU		14	/* .equ, .gblequ, .lclequ */
#define	  O_EQU      0		/* .equ */
#define	  O_GBLEQU   1		/* .gblequ */
#define	  O_LCLEQU   2		/* .lclequ */
#define	S_DATA		15	/* .byte, .word, long, .3byte, .4byte, .db, .dw, .dl, .fcb, .fdb */
#define	  O_1BYTE    1		/* .byte, .db, .fcb */
#define	  O_2BYTE    2		/* .word, .dw, .fdb */
#define	  O_3BYTE    3		/* .3byte */
#define	  O_4BYTE    4		/* .4byte, .long, .dl */
#define	S_BLK		16	/* .blkb, .blkw, .blkl, .blk3, .blk4, .ds, .rmb, .rs */
/*	  O_1BYTE    1	*/	/* .blkb, .ds, .rmb, .rs */
/*	  O_2BYTE    2	*/	/* .blkw */
/*	  O_3BYTE    3	*/	/* .blk3 */
/*	  O_4BYTE    4	*/	/* .blk4, .blkl */
#define	S_ASCIX		17	/* .ascii, .ascis, .asciz, .str, .strs, .strz */
#define	  O_ASCII    0		/* .ascii */
#define	  O_ASCIS    1		/* .ascis */
#define	  O_ASCIZ    2		/* .asciz */
#define	S_DEFINE	18	/* .define, .undefine */
#define	  O_DEF      0		/* .define */
#define	  O_UNDEF    1		/* .undefine */
#define	S_BOUNDARY	19	/* .even, .odd */
#define	  O_EVEN     0		/* .even */
#define	  O_ODD      1		/* .odd */
#define	  O_BNDRY    2		/* .bndry */
#define	S_MSG		20	/* .msg */
#define	S_ERROR		21	/* .assume, .error */
#define	  O_ASSUME   0		/* .assume */
#define	  O_ERROR    1		/* .error */
#define	S_MSB		22	/* .msb(0), .msb(1), .msb(2), .msb(3), .lohi, .hilo */
#define	  O_MSB      0		/* .msb(0), .msb(1), .msb(2), .msb(3) */
#define	  O_LOHI     1		/* .lohi */
#define	  O_HILO     2		/* .hilo */
#define	S_BITS		23	/* .8bit, .16bit, .24bit, .32bit */
/*	  O_1BYTE    1	*/	/* .8bit */
/*	  O_2BYTE    2	*/	/* .16bit */
/*	  O_3BYTE    3	*/	/* .24bit */
/*	  O_4BYTE    4	*/	/* .32bit */
#define	S_END		24	/* .end */
#define	S_MACRO		25	/* .macro, .endm, .mexit, ... */
#define	  O_MACRO    0		/* .macro */
#define	  O_ENDM     1		/* .endm */
#define	  O_MEXIT    2		/* .mexit */
#define	  O_NCHR     3		/* .nchr */
#define	  O_NARG     4		/* .narg */
#define	  O_NTYP     5		/* .ntyp */
#define	  O_IRP	     6		/* .irp */
#define	  O_IRPC     7		/* .irpc */
#define	  O_REPT     8		/* .rept */
#define	  O_NVAL     9		/* .nval */
#define	  O_MDEL     10		/* .mdelete */
#define	  O_CHECK    255	/* Building/Exiting a Macro Check */
#define S_TRACE		26	/* .trace, .ntrace */
#define   O_TRC      0		/* .trace */
#define   O_NTRC     1		/* .ntrace */
#define S_CONST		27	/* Permanent Symbols Accessible As Constants */
/*			28  */	/* Spare Definition */
/*			29  */	/* Spare Definition */

#define S_DIREOL	30	/* Assembler Directive End Of List */

/* sdas specific */
#define S_FLOAT         32      /* .df */
#define S_ULEB128       33      /* .uleb128 */
#define S_SLEB128       34      /* .sleb128 */
#define S_OPTSDCC       35      /* .optsdcc */
/* end sdas specific */

/*
 *	The tsym structure is a linked list of temporary
 *	symbols defined in the assembler source files following
 *	a normal symbol.  The structure contains the temporary
 *	symbols number, a flag (multiply defined), a pointer to the
 *	area structure defining where the temporary structure
 *	is located, and the temporary symbol's address relative
 *	to the base address of the area where the symbol
 *	is located.
 */
struct	tsym
{
	struct	tsym *t_lnk;	/* Link to next */
	a_uint	t_num;		/* 0-65535$      for a 16-bit int */
				/* 0-4294967295$ for a 32-bit int */
	int	t_flg;		/* flags */
	struct	area *t_area;	/* Area */
	a_uint	t_addr;		/* Address */
};

/*
 *	The bank structure contains the parameter values for a
 *	specific collection of areas.  The bank structure
 *	is a linked list of banks.  The initial default bank
 *	is "_CODE" defined in ___pst.c, the next bank structure
 *	will be linked to this structure through the structure
 *	element 'struct bank *b_bp'.  The structure contains the
 *	bank name, bank reference number ("_CODE" is 0) determined
 *	by the order of .bank directives, the bank base address
 *	(default = 0), bank size (default = 0, whole addressing space),
 *	bank mapping parameter, and output data file suffix
 *	(appended as a suffix to the output file name)
 *	are optional parameters of the .bank assembler
 *	directive which are passed to the linker as the
 *	default link parameters, and the bank flags which specify
 *	what options have been specified.
 */
struct	bank
{
	struct	bank *b_bp;	/* Bank link */
	char *	b_id;		/* Bank Name */
	char *	b_fsfx;		/* Bank File Suffix */
	int	b_ref;		/* Ref. number */
	a_uint	b_base;		/* Bank base address */
	a_uint	b_size;		/* Bank size */
	a_uint	b_map;		/* Bank mapping */
	int	b_flag;		/* Bank flags */
};

#define B_BASE	0001		/* 'base' address specified */
#define B_SIZE	0002		/* 'size' of bank specified */
#define	B_FSFX	0004		/* File suffix specified */
#define	B_MAP	0010		/* Mapped Bank Flag */

/*
 *	The def structure is used by the .define assembler
 *	directive to define a substitution string for a
 *	single word.  The def structure contains the
 *	string being defined, the string to substitute
 *	for the defined string, and a link to the next
 *	def structure.  The defined string is a sequence
 *	of characters not containing any white space
 *	(i.e. NO SPACEs or TABs).  The substitution string
 *	may contain SPACES and/or TABs.
 */
struct def
{
	struct def	*d_dp;		/* link to next define */
	char		*d_id;		/* defined string */
	char		*d_define;	/* string to substitute for defined string */
	int		d_dflag;	/* (1) .defined / (0) .undefined */
};

/*
 *	The mode structure contains the specification of one of the
 *	assemblers' merge modes.  Each assembler must specify
 *	at least one merge mode.  The merging specification
 *	allows arbitrarily defined active bits and bit positions.
 *	The 32 element arrays are indexed from 0 to 31.
 *	Index 0 corresponds to bit 0, ..., and 31 corresponds to bit 31
 *	of a normal integer value.
 *
 *	The value of the element specifies if the normal integer bit
 *	is active (bit <7> is set, 0x80) and what destination bit
 *	(bits <4:0>, 0 - 31) should be loaded with this normal
 *	integer bit.
 *
 *	The specification for a 32-bit integer:
 *
 *	char mode_[32] = {
 *		'\200',	'\201',	'\202',	'\203',	'\204',	'\205',	'\206',	'\207',
 *		'\210',	'\211',	'\212',	'\213',	'\214',	'\215',	'\216',	'\217',
 *		'\220',	'\221',	'\222',	'\223',	'\224',	'\225',	'\226',	'\227',
 *		'\230',	'\231',	'\232',	'\233',	'\234',	'\235',	'\236',	'\237'
 *	};
 *
 *
 *	The specification for the 11-bit 8051 addressing mode:
 *
 *	char mode_[32] = {
 *		'\200',	'\201',	'\202',	'\203',	'\204',	'\205',	'\206',	'\207',
 *		'\215',	'\216',	'\217',	'\013',	'\014',	'\015',	'\016',	'\017',
 *		'\020',	'\021',	'\022',	'\023',	'\024',	'\025',	'\026',	'\027',
 *		'\030',	'\031',	'\032',	'\033',	'\034',	'\035',	'\036',	'\037'
 *	};
 *
 *
 *     *m_def is a pointer to the bit relocation definition.
 *	m_flag indicates that bit position swapping is required.
 *	m_dbits contains the active bit positions for the output.
 *	m_sbits contains the active bit positions for the input.
 */
struct	mode
{
	char *	m_def;		/* Bit Relocation Definition */
	a_uint	m_flag;		/* Bit Swapping Flag */
	a_uint	m_dbits;	/* Destination Bit Mask */
	a_uint	m_sbits;	/* Source Bit Mask */
};

/*
 * Definitions for Character Types
 */
#define	SPACE	'\000'
#define ETC	'\000'
#define	LETTER	'\001'
#define	DIGIT	'\002'
#define	BINOP	'\004'
#define	RAD2	'\010'
#define	RAD8	'\020'
#define	RAD10	'\040'
#define	RAD16	'\100'
#define	ILL	'\200'

#define	DGT2	(DIGIT|RAD16|RAD10|RAD8|RAD2)
#define	DGT8	(DIGIT|RAD16|RAD10|RAD8)
#define	DGT10	(DIGIT|RAD16|RAD10)
#define	LTR16	(LETTER|RAD16)

/*
 *	The expr structure is used to return the evaluation
 *	of an expression.  The structure supports three valid
 *	cases:
 *	(1)	The expression evaluates to a constant,
 *		mode = S_USER, flag = 0, addr contains the
 *		constant, and base = NULL.
 *	(2)	The expression evaluates to a defined symbol
 *		plus or minus a constant, mode = S_USER,
 *		flag = 0, addr contains the constant, and
 *		base = pointer to area symbol.
 *	(3)	The expression evaluates to an external
 *		global symbol plus or minus a constant,
 *		mode = S_NEW, flag = 1, addr contains the
 *		constant, and base = pointer to symbol.
 */
struct	expr
{
	char	e_mode;		/* Address mode */
	char	e_flag;		/* Symbol flag */
	a_uint	e_addr;		/* Address */
	union	{
		struct area *e_ap;
		struct sym  *e_sp;
	} e_base;		/* Rel. base */
        int     e_rlcf;         /* Rel. flags */
};

/*
 *	The asmf structure contains the information
 *	pertaining to an assembler source file/macro.
 *
 * The Parameters:
 *	next	is a pointer to the next object in the linked list
 *	objtyp	specifies the object type - T_ASM, T_INCL, T_MACRO
 *	line	is the saved line number of the parent object
 *	flevel	is the saved flevel of the parent object
 *	tlevel	is the saved tlevel of the parent object
 *	lnlist	is the saved lnlist of the parent object
 *	fp	is the source FILE handle
 *	afp	is the file path length (excludes the files name.ext)
 *	afn[]	is the assembler/include file path/name.ext
 */
struct	asmf
{
	struct	asmf *next;	/* Link to Next Object */
	int	objtyp;		/* Object Type */
	int	line;		/* Saved Line Counter */
	int	flevel;		/* saved flevel */
	int	tlevel;		/* saved tlevel */
	int	lnlist;		/* saved lnlist */
	FILE *	fp;		/* FILE Handle */
	int	afp;		/* File Path Length */
	char	afn[FILSPC];	/* File Name */
};

/*
 *	The macrofp structure masquerades as a FILE Handle
 *	for inclusion in an asmf structure.  This structure
 *	contains the reference to the macro to be inserted
 *	into the assembler stream and information to 
 *	restore the assembler state at macro completion.
 *
 * The Parameters:
 *	np	is a pointer to the macro definition
 *	lstptr	is a pointer to the next macro line
 *	rptcnt	is the macro repeat counter
 *	rptidx	is the current repeat count
 *	flevel	is the saved assembler flevel
 *	tlevel	is the saved assembler tlevel
 *	lnlist	is the saved assembler lnlist
 *	npexit	non zero if an .mexit is encountered
 */
struct	macrofp {
	struct mcrdef *	np;		/* pointer to macro definition */
	struct strlst *	lstptr;		/* pointer to next line of macro */
	int		rptcnt;		/* repeat counter */
	int		rptidx;		/* repeat index */
	int		flevel;		/* saved flevel */
	int		tlevel;		/* saved tlevel */
	int		lnlist;		/* saved lnlist */
	int		npexit;		/* .mexit called */
};

/*
 *	The mcrdef structure contains the
 *	information about a macro definition.
 *
 *	When the macro is defined the definition
 *	arguments are packed into a linked list of
 *	strings beginning with bgnarg and ending with
 *	endarg. The number of args is placed in narg.
 *
 *	When the macro is invoked the expansion
 *	argument strings are placed into a linked
 *	list of strings beginning with bgnxrg and
 *	ending with endxrg. The number of expansion
 *	arguments is placed in xarg.
 *
 * The Parameters:
 *	next	is a pointer to the next macro definition structure
 *	name	is a pointer to the macro name string
 *	bgnlst	is a pointer to the first text line of the macro
 *	endlst	is a pointer to the last  text line of the macro
 *	type	is the macro type - .macro, .irp, .irpc, or .rept
 *	rptcnt	is the repeat count for the macro
 *	nest	is the macro nesting counter
 *	narg	is the number of macro definition arguments
 *	bgnarg	is a pointer to the first definition argument string
 *	endarg	is a pointer to the last  definition argument string
 *	xarg	is the number of expansion arguments at macro invocation
 *	bgnxrg	is a pointer to the first expansion argument string
 *	endxrg	is a pointer to the last  expansion argument string
 */
struct	mcrdef {
	struct mcrdef *	next;		/* link to next macro definition */
	char *		name;		/* pointer to the macro name */
	struct strlst *	bgnlst;		/* link to first text line of macro */
	struct strlst *	endlst;		/* link to last text line of macro */
	int		type;		/* macro type */
	int		rptcnt;		/* repeat counter */
	int		nest;		/* macro nesting counter */
	int		narg;		/* number of macro definition arguments */
	struct strlst * bgnarg;		/* link to first macro definition argument */
	struct strlst * endarg;		/* link to last macro definition argument */
	int		xarg;		/* number of macro expansion arguments */
	struct strlst * bgnxrg;		/* link to first macro expansion argument */
	struct strlst * endxrg;		/* link to last macro expansion argument */
};

/*
 *	The strlst structure is a linked list of strings.
 *
 * The Parameters:
 *	next	is a pointer to the next string.
 *	text	is a pointer to a text string.
 */
struct	strlst {
	struct strlst *	next;		/* pointer to next string */
	char *		text;		/* pointer to string text */
};

/*
 *	The mstack structure contains parameters for:
 *	(1) - Recovery of memory used by a macro invocation
 *	(2) - Restoration of recursed argument parameters
 *
 * The Parameters:
 *	mcrcnt	is the number of macros created (excludes incline macros)
 *	mcrmem	is a pointer to the current allocated memory block
 *	pnext	is a pointer to the next available memory byte in the memory block
 *	bytes	is the number of bytes available in the memory block
 *	xarg	is the number of expansion arguments at macro invocation
 *	bgnxrg	is a pointer to the first expansion argument string
 *	endxrg	is a pointer to the last  expansion argument string
 */
struct mstack {
	int		mcrcnt;		/* macros created */
	struct memlnk *	mcrmem;		/* pointer to memory block */
	void	      * pnext;		/* pointer to available bytes */
	int		bytes;		/* bytes available */
	int		xarg;		/* number of macro expansion arguments */
	struct strlst * bgnxrg;		/* link to first macro expansion argument */
	struct strlst * endxrg;		/* link to last macro expansion argument */
};

/*
 *	The memlnk structure is a linked list
 *	of memory allocations.
 *
 *	The function new() uses the memlnk structure
 *	to create a linked list of allocated memory
 *	that can be traversed by asfree() to release
 *	the allocated memory.
 *
 *	The function mhunk() uses the memlnk structure
 *	to create a linked list of allocated memory
 *	that can be reused.
 *
 * The Parameters:
 *	next	is a pointer to the next memlnk structure.
 *	ptr	is a pointer to the allocated memory.
 */
struct	memlnk {
	struct memlnk *	next;		/* link to next memlnk */
	void *		ptr;		/* pointer to allocated memory */
};

/*
 *	External Definitions for all Global Variables
 */

extern	int	aserr;		/*	ASxxxx error counter
				 */
extern	int	trcflags;	/*	ASxxxx tracing flags
				 */
extern	jmp_buf	jump_env;	/*	compiler dependent structure
				 *	used by setjmp() and longjmp()
				 */
extern	struct	asmf	*asmc;	/*	Pointer to the current
				 *	source input structure
				 */
extern	struct	asmf	*asmo;	/*	The pointer to the first
				 *	command line insert structure
				 */
extern	struct	asmf	*asmp;	/*	The pointer to the first assembler
				 *	source file structure of a linked list
				 */
extern	struct	asmf	*asmi;	/*	Queued pointer to an include file
				 *	source input structure
				 */
extern	struct	asmf	*asmq;	/*	Queued pointer to a macro
				 *	source input structure
				 */
extern	struct mcrdef *	mcrlst;	/*	link to list of defined macros
				 */
extern	struct mcrdef *	mcrp;	/*	link to list of defined macros
				 */
extern	struct memlnk *	asxmem;	/*	Assembler Memory Allocation Structure
				 */
extern	struct memlnk *	pmcrmem;/*	First Macro Memory Allocation Structure
				 */
extern	struct memlnk *	mcrmem;	/*	Macro Memory Allocation Structure
				 */
extern	int	mcrcnt;		/*	Regular Macros Created
				 */
extern	int	mcrexe;		/*	Regular Macros Executed
				 */
extern	int	inlcnt;		/*	Inline Macros Created
				 */
extern	int	inlexe;		/*	Inline Macros Executed
				 */
extern	int	mlevel;		/*	Macro Stack Level
				 */
extern	struct mstack mstk[MAXMCR]; /*	Macro Stack
				 */
extern	int	alevel;		/*	area stack pointer
				 */
extern	struct	area *astack[16]; /*	area stack
				 */
extern	int	asmblk;		/*	Assembler data blocks allocated
				 */
extern	int	mcrblk;		/*	Macro data blocks allocated
				 */
extern	int	incfil;		/*	include file nesting counter
				 */
extern	int	maxinc;		/*	maximum include file nesting encountered
				 */
extern	int	mcrfil;		/*	macro nesting counter
				 */
extern	int	maxmcr;		/*	maximum macro nesting emcountered
				 */
extern	int	flevel;		/*	IF-ELSE-ENDIF flag (false != 0)
				 */
extern	int	ftflevel;	/*	IIFF-IIFT-IIFTF FLAG
				 */
extern	int	tlevel;		/*	current conditional level
				 */
extern	int	lnlist;		/*	LIST-NLIST options
				 */
extern	int	ifcnd[MAXIF+1];	/*	array of IF statement condition
				 *	values (0 = FALSE) indexed by tlevel
				 */
extern	int	iflvl[MAXIF+1];	/*	array of IF-ELSE-ENDIF flevel
				 *	values indexed by tlevel
				 */
extern	char	afn[FILSPC];	/*	current input file specification
				 */
extern	int	afp;		/*	current input file path length
				 */
extern	char	afntmp[FILSPC];	/*	temporary input file specification
				 */
extern	int	afptmp;		/*	temporary input file path length
				 */
extern	int	srcline;	/*	current source line number
				 */
extern	int	asmline;	/*	current assembler file line number
				 */
extern	int	incline;	/*	current include file line number
				 */
extern	int	mcrline;	/*	current macro line number
				 */
extern	int	radix;		/*	current number conversion radix:
				 *	2 (binary), 8 (octal), 10 (decimal),
				 *	16 (hexadecimal)
				 */
extern	int	line;		/*	current assembler source line number
				 */
extern	int	page;		/*	current page number
				 */
extern	int	lop;		/*	current line number on page
				 */
extern	time_t	curtim;		/*	pointer to the current time string
				 */
extern	int	pass;		/*	assembler pass number
				 */
extern	int	aflag;		/*	-a, make all symbols global flag
				 */
extern	int	bflag;		/*	-b(b), listing mode flag
				 */
extern	int	cflag;		/*	-c, disable cycle counts in listing flag
				 */
extern	int	fflag;		/*	-f(f), relocations flagged flag
				 */
extern	int	gflag;		/*	-g, make undefined symbols global flag
				 */
extern	int	iflag;		/*	-i, insert command line string flag
				 */

#if NOICE
extern	int	jflag;		/*	-j, enable NoICE Debug Symbols
				 */
#endif

extern	int	kflag;		/*	-k, disable error output to .lst file
				 */
extern	int	lflag;		/*	-l, generate listing flag
				 */
extern  int     nflag;          /*      -n, don't resolve global symbols flag
                                 */
extern	int	oflag;		/*	-o, generate relocatable output flag
				 */
extern	int	pflag;		/*	-p, disable listing pagination
				 */
extern	int	rflag;		/*	-r, put line numbers in hint file for .lst to .rst
				 */
extern	int	sflag;		/*	-s, generate symbol table flag
				 */
extern	int	tflag;		/*	-t, output diagnostic parameters from assembler
				 */
extern	int	uflag;		/*	-u, disable .list/.nlist processing flag
				 */
extern	int	vflag;		/*	-v, enable out of range signed / unsigned errors
				 */
extern	int	wflag;		/*	-w, enable wide listing format
				 */
extern	int	xflag;		/*	-x, listing radix flag
				 */

#if SDCDB
extern	int	yflag;		/*	-y, enable SDCC Debug Symbols
				 */
#endif

extern	int	zflag;		/*	-z, disable symbol case sensitivity
				 */
extern  int     waddrmode;      /*      WORD Address mode flag
				 */
extern	int	a_bytes;	/*	REL file T Line address length
				 */
extern	a_uint	a_mask;		/*	Address Mask
				 */
extern	a_uint	s_mask;		/*	Sign Mask
				 */
extern	a_uint	v_mask;		/*	Value Mask
				 */
extern	a_uint	p_mask;		/*	Page Mask
				 */
extern	int	as_msb;		/*	current MSB byte select
				 *	0 == low byte
				 *	1 == high byte
				 *	2 == third byte
				 *	3 == fourth byte
				 */
extern	a_uint	laddr;		/*	address of current assembler line,
				 *	equate, or value of .if argument
				 */
extern	a_uint	fuzz;		/*	tracks pass to pass changes in the
				 *	address of symbols caused by
				 *	variable length instruction formats
				 */
extern	int	lmode;		/*	listing mode
				 */
extern	char *	eqt_area;	/*	pointer to the area name
				 *	associated with lmode = ELIST
				 */
extern	char	txt[NTXT];	/*	T Line Values
				 */
extern	char	rel[NREL];	/*	R Line Values
				 */
extern	char	*txtp;		/*	Pointer to T Line Values
				 */
extern	char	*relp;		/*	Pointer to R Line Values
				 */
extern	struct	area	*areap;	/*	pointer to an area structure
				 */
extern	struct	bank	*bankp;	/*	pointer to a bank structure
				 */
extern  struct  area    area[]; /*      array of 1 area
                                 */
extern	struct	def	*defp;	/*	pointer to a def structure
				 */
extern	struct	sym	sym[];	/*	array of 1 symbol
				 */
extern	struct	sym	*symp;	/*	pointer to a symbol structure
				 */
extern	struct	sym *symhash[NHASH]; /*	array of pointers to NHASH
				      *	linked symbol lists
				      */
extern	struct	mne *mnehash[NHASH]; /*	array of pointers to NHASH
				      *	linked mnemonic/directive lists
				      */
extern	char	*ep;		/*	pointer into error list
				 *	array eb[NERR]
				 */
extern	char	eb[NERR];	/*	array of generated error codes
				 */
extern	char	*ex[NERR];	/*	array of error string pointers
				 */
extern	int	exmode;		/*	expanded error code mode
				 */
extern	char	*ip;		/*	pointer into the assembler-source
				 *	text line in ib[]
				 */
extern  char    *ib;            /*      assembler-source text line for processing
				 */
extern  char    *ic;            /*      assembler-source text line for listing
                                 */
extern	char	*il;		/*	pointer to the assembler-source
				 *	text line to be listed
				 */
extern	char	*cp;		/*	pointer to assembler output
				 *	array cb[]
				 */
extern	char	cb[NCODE];	/*	array of assembler output values
				 */
extern	int	*cpt;		/*	pointer to assembler relocation type
				 *	output array cbt[]
				 */
extern	int	cbt[NCODE];	/*	array of assembler relocation types
				 *	describing the data in cb[]
				 */
extern	int	awg;		/*	automatic word generation (special coding)
				 *	0 - default not defined, 1-4 - 1-4 bytes
				 */
extern	int	csn;		/*	'C' Style Numbers - 0nnn (Octal), 0xnnn (Hex), Else Decimal
				 *	0 - default disabled, 1 - enabled
				 */
extern	int	opcycles;	/*	opcode execution cycles
				 */
extern	char	tb[NTITL];	/*	Title string buffer
				 */
extern	char	stb[NSBTL];	/*	Subtitle string buffer
				 */
extern  char    erb[NINPUT+4];  /*      Error string buffer
				 */
extern	char	symtbl[];	/*	string "Symbol Table"
				 */
extern	char	aretbl[];	/*	string "Area Table"
				 */
extern	char	module[NCPS+2];	/*	module name string
				 */
extern	FILE	*hfp;		/*	.lst to .rst hint file handle
				 */
extern	FILE	*lfp;		/*	list output file handle
				 */
extern	FILE	*ofp;		/*	relocation output file handle
				 */
extern	FILE	*tfp;		/*	symbol table output file handle
				 */
extern  unsigned char ctype[256]; /*    array of character types, one per
                                 *      ASCII/OEM character
				 */
extern  char    ccase[256];     /*      an array of characters which
				 *	perform the case translation function
				 */
/*sdas specific */
extern  int     asfatal;        /*      ASxxxx fatal error counter
				 */
extern  int     org_cnt;        /*      .org directive counter
				 */
extern  char    *optsdcc;       /*      sdcc compile options
				 */
/*end sdas specific */

/* C Library functions */
/* for reference only
extern	int		fclose();
extern	char *		fgets();
extern	FILE *		fopen();
extern	int		fprintf();
extern	void		free();
extern	void		longjmp();
extern	void *		malloc();
extern	int		printf();
extern	char		putc();
extern	int		rewind();
extern	int		setjmp();
extern	int		strchr();
extern	int		strcmp();
extern	char *		strcpy();
extern	int		strlen();
extern	char *		strncpy();
extern	char *		strrchr();
*/

/* Machine independent functions */

/* C Library functions */
/* extern	void		exit(int n); */

/* asmain.c */
extern	FILE *		afile(char *fn, char *ft, int wf);
extern  void            afilex(char *fn, char *ft);
extern	void		asexit(int i);
extern	void		asmbl(void);
extern	void		boundary(a_uint n);
extern	void		equate(char *id,struct expr *e1,a_uint equtype);
extern	char *		filespec(int argc, char *argv[], int *i, char *p, char **nam, char **ext, int opt);
extern	void		fixrelfil(char *p, char **relfil);
extern	int		fndidx(char *str);
extern	int		intsiz(void);
extern	void		insline(char *str, int i);
/*
extern	int		main(int argc, char *argv[]);
*/
extern	void		multipass(void);
extern	void		newdot(struct area *nap);
extern	void		pckchr(a_uint size, int c, a_uint *v, a_uint *cnt);
extern	void		phase(struct area *ap, a_uint a);
extern	char *		usetxt[];
extern	void		usage(void);

/* asmcro.c */
extern	char *		fgetm(char *ptr, int len, FILE *fp);
extern	void		getdarg(struct mcrdef *np);
extern	void		getdfarg(struct mcrdef *np);
extern	void		getinline(struct mcrdef *np);
extern	void		getxarg(struct mcrdef *np);
extern	void		getxnum(char *id);
extern	void		getxstr(char *id);
extern	void		macro(struct mcrdef * np);
extern	void		macroscn(struct macrofp *nfp);
extern	int		macrosub(char *id, struct macrofp *nfp);
extern	void		mcrinit(void);
extern	int		mcrprc(int code);
extern	void *		mhunk(void);
extern	char *		mstring(char *str);
extern	char *		mstruct(int n);
extern	struct mcrdef *	newdef(int code, char *id);
extern	struct mcrdef *	nlookup(char *id);
extern	void		pshmstk(struct mcrdef *np);
extern	void		popmstk(struct mcrdef *np);

/* aslex.c */
extern	void		chopcrlf(char *str);
extern	int		comma(int flag);
extern	char		endline(void);
extern	int		get(void);
extern	int		getdlm(void);
extern	void		getdstr(char *str, int slen);
extern	void		getid(char *id, int c);
extern	int		getmap(int d);
extern	int		getnb(void);
extern	int		getlnm(void);
extern	void		getst(char *id, int c);
extern	int		more(void);
extern	int		nxtline(void);
extern	int		replace(char *id);
extern	void		scanline(void);
extern	void		unget(int c);
extern	int		skpcomma(void);

/* assym.c */
extern	void		allglob(void);
extern	struct	area *	alookup(char *id);
extern	void		asfree(void);
extern	struct	bank *	blookup(char *id);
extern	struct	def *	dlookup(char *id);
extern  int             hash(const char *p, int flag);
extern  struct  sym *   lookup(const char *id);
extern	struct	mne *	mlookup(char *id);
extern	char *		new(unsigned int n);
extern	struct	sym *	slookup(char *id);
extern  char *          strsto(const char *str);
extern  int             symeq(const char *p1, const char *p2, int flag);
extern	void		syminit(void);
extern	void		symglob(void);

/* assubr.c */
extern	void		aerr(void);
extern	void		diag(void);
extern	void		err(int c);
extern	void		xerr(int c, char *str);
extern	char *		errors[];
extern	char *		geterr(int c);
extern	void		qerr(void);
extern	void		rerr(void);
/* sdas specific */
extern  void            warnBanner(void);
/* end sdas specific */

/* asexpr.c */
extern	void		abscheck(struct expr *esp);
extern	a_uint		absexpr(void);
extern	void		clrexpr(struct expr *esp);
extern	int		digit(int c, int r);
extern	int		is_digit(int c, int r);
extern	void		exprmasks(int n);
extern	void		expr(struct expr *esp, int n);
extern	void		binop(int c, struct expr *esp, struct expr *re);
extern	int		is_abs(struct expr *esp);
extern	int		oprio(int c);
extern	a_uint		rngchk(a_uint n);
extern	void		term(struct expr *esp);

/* asdbg */
extern  char *          BaseFileName(struct asmf *currFile, int spacesToUnderscores);
extern	void		DefineNoICE_Line(void);
extern	void		DefineSDCC_Line(void);

/* aslist.c */
extern	void		list(void);
extern  void            list1(char *wp, int *wpt, int nb, int n, int f, int g);
extern	void		list2(int t);
extern	void		listhlr(int hlr_lst, int hlr_mode, int hlr_nb);
extern	void		lstsym(FILE *fp);
extern	void		slew(FILE *fp, int flag);

/* asout.c */
extern	int		lobyte(a_uint v);
extern	int		hibyte(a_uint v);
extern	int		thrdbyte(a_uint v);
extern	int		frthbyte(a_uint v);
extern	void		out(char *p, int n);
extern	void		outarea(struct area *ap);
extern	void		outbank(struct bank *bp);
extern	void		outmode(int index, struct mode *sdp);
extern	void		outmline(char *p, char *q);
extern	void		outdp(struct area *carea, struct expr *esp, int r);
extern	void		outall(void);
extern	void		outdot(void);
extern	void		outbuf(char *s);
extern	void		outchk(int nt, int nr);
extern  void            outradix(void);
extern	void		outgsd(void);
extern	void		outsym(struct sym *sp);
extern	void		outab(a_uint v);
extern	void		outaw(a_uint v);
extern	void		outa3b(a_uint v);
extern	void		outa4b(a_uint v);
extern	void		outaxb(int i, a_uint v);
extern	void		outatxb(int i, a_uint v);
extern	void		outrb(struct expr *esp, int r);
extern	void		outrw(struct expr *esp, int r);
extern	void		outr3b(struct expr *esp, int r);
extern	void		outr4b(struct expr *esp, int r);
extern	void		outrxb(int i, struct expr *esp, int r);
extern	void		outrbm(struct expr *esp, int r, a_uint v);
extern	void		outrwm(struct expr *esp, int r, a_uint v);
extern  void            outrwp(struct expr *esp, a_uint op, a_uint mask, int jump);
extern	void		outr3bm(struct expr *esp, int r, a_uint v);
extern	void		outr4bm(struct expr *esp, int r, a_uint v);
extern	void		outrxbm(int i, struct expr *esp, int r, a_uint v);
extern	a_uint		outmerge(a_uint esp, int r, a_uint v);
extern	void		out_lb(a_uint v, int t);
extern	void		out_lw(a_uint v, int t);
extern	void		out_l3b(a_uint v, int t);
extern	void		out_l4b(a_uint v, int t);
extern	void		out_lxb(int i, a_uint v, int t);
extern	void		out_rw(a_uint v);
extern	void		out_txb(int i, a_uint v);

/* Machine dependent variables */

extern	char *		cpu;
extern	char *		dsft;
extern	struct	area	area[];
extern	struct	bank	bank[];
extern	struct	mne	mne[];
extern	struct	mode *	modep[16];

/* Machine dependent functions */

extern	void		machine(struct mne *mp);
extern	void		minit(void);

/*
 *	The number of cycle count digits to display.
 *	The valid range is bounded be >= 2 and <= 4.
 *	An assembler requiring other than the default
 *	of 2 digits should set this value in minit().
 */
extern	int		cycldgts;

/*
 *	A pointer to the external function
 *	which provides auxiliary functionallity
 *	to the term() function in asexpr.c
 *	returns:	!0 if item processed
 *			 0 if nothing processed
 */
extern	int		(*mchterm_ptr)(struct expr *esp);

/*
 *	A pointer to the external function which
 *	provides auxiliary functionallity to the
 *	.enabl/.dsabl processing in asmain.c
 *	returns:	!0 if item processed
 *			 0 if nothing processed
 */
extern	int		(*mchoptn_ptr)(char *id, int v);

/*
 *	Parameters which specify the optional multi-pass
 *	processing for assemblers that have variable
 *	length instructions.
 *	An assembler requiring more than the default
 *	number of passes required to resolve all forward
 *	references must set passlmt to a number greater
 *	than 0 to enable the multi-pass processing.
 */
extern	int	nflglmt;
extern	int	passlmt;
extern	int	passcnt;
extern	int	passJLH;
extern	int	passfuz;

/* asxcnv.c */

/* Defined under asmain.c above
extern	void		asexit(int i);
extern	int		main(int argc, char *argv[]);
extern	char *		usetxt[];
extern	void		usage(void);
*/
extern	void		linout(char *str, unsigned int n);

/* asxscn.c */

/* Defined under asmain.c above
extern	void		asexit(int i);
extern	int		main(int argc, char *argv[]);
extern	char *		usetxt[];
extern	void		usage(void);
*/
extern	int		dgt(int rdx, char *str, int n);

/* sdas specific */
/* strcmpi.c */
extern  int as_strcmpi(const char *s1, const char *s2);
extern  int as_strncmpi(const char *s1, const char *s2, size_t n);
/* end sdas specific */

