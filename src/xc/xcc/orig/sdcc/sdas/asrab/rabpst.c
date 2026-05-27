/* rabpst.c */

/*
 *  Copyright (C) 1989-2023  Alan R. Baldwin
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
 * ported to the Rabbit2000 by
 * Ulrich Raich and Razaq Ijoduola
 * PS Division
 * CERN
 * CH-1211 Geneva-23
 * email: Ulrich dot Raich at cern dot ch
 */

/*
 * Extensions: P. Felber
 * 
 * Altered by Leland Morrison to support rabbit 2000 
 *   and rabbit 4000 instruction sets (2011)
 */

#include "asxxxx.h"
#include "rab.h"

/*
 * Mnemonic Structure
 */
struct	mne	mne[] = {

        /* machine */

        /* system */

    {   NULL,   "CON",          S_ATYP,         0,      A_CON   },
    {   NULL,   "OVR",          S_ATYP,         0,      A_OVR   },
    {   NULL,   "REL",          S_ATYP,         0,      A_REL   },
    {   NULL,   "ABS",          S_ATYP,         0,      A_ABS   },
    {   NULL,   "NOPAG",        S_ATYP,         0,      A_NOPAG },
    {   NULL,   "PAG",          S_ATYP,         0,      A_PAG   },

    {	NULL,	".page",	S_PAGE,		0,	0	},
    {	NULL,	".title",	S_HEADER,	0,	O_TITLE	},
    {	NULL,	".sbttl",	S_HEADER,	0,	O_SBTTL	},
    {	NULL,	".module",	S_MODUL,	0,	0	},
    {	NULL,	".include",	S_INCL,		0,	I_CODE	},
    {	NULL,	".incbin",	S_INCL,		0,	I_BNRY	},
    {	NULL,	".area",	S_AREA,		0,	0	},
    {	NULL,	".org",		S_ORG,		0,	0	},
    {	NULL,	".radix",	S_RADIX,	0,	0	},
    {	NULL,	".globl",	S_GLOBL,	0,	0	},
    {	NULL,	".local",	S_LOCAL,	0,	0	},
    {	NULL,	".if",		S_CONDITIONAL,	0,	O_IF	},
    {	NULL,	".iff",		S_CONDITIONAL,	0,	O_IFF	},
    {	NULL,	".ift",		S_CONDITIONAL,	0,	O_IFT	},
    {	NULL,	".iftf",	S_CONDITIONAL,	0,	O_IFTF	},
    {	NULL,	".ifdef",	S_CONDITIONAL,	0,	O_IFDEF	},
    {	NULL,	".ifndef",	S_CONDITIONAL,	0,	O_IFNDEF},
    {	NULL,	".ifgt",	S_CONDITIONAL,	0,	O_IFGT	},
    {	NULL,	".iflt",	S_CONDITIONAL,	0,	O_IFLT	},
    {	NULL,	".ifge",	S_CONDITIONAL,	0,	O_IFGE	},
    {	NULL,	".ifle",	S_CONDITIONAL,	0,	O_IFLE	},
    {	NULL,	".ifeq",	S_CONDITIONAL,	0,	O_IFEQ	},
    {	NULL,	".ifne",	S_CONDITIONAL,	0,	O_IFNE	},
    {	NULL,	".ifb",		S_CONDITIONAL,	0,	O_IFB	},
    {	NULL,	".ifnb",	S_CONDITIONAL,	0,	O_IFNB	},
    {	NULL,	".ifidn",	S_CONDITIONAL,	0,	O_IFIDN	},
    {	NULL,	".ifdif",	S_CONDITIONAL,	0,	O_IFDIF	},
    {	NULL,	".iif",		S_CONDITIONAL,	0,	O_IIF	},
    {	NULL,	".iiff",	S_CONDITIONAL,	0,	O_IIFF	},
    {	NULL,	".iift",	S_CONDITIONAL,	0,	O_IIFT	},
    {	NULL,	".iiftf",	S_CONDITIONAL,	0,	O_IIFTF	},
    {	NULL,	".iifdef",	S_CONDITIONAL,	0,	O_IIFDEF},
    {	NULL,	".iifndef",	S_CONDITIONAL,	0,	O_IIFNDEF},
    {	NULL,	".iifgt",	S_CONDITIONAL,	0,	O_IIFGT	},
    {	NULL,	".iiflt",	S_CONDITIONAL,	0,	O_IIFLT	},
    {	NULL,	".iifge",	S_CONDITIONAL,	0,	O_IIFGE	},
    {	NULL,	".iifle",	S_CONDITIONAL,	0,	O_IIFLE	},
    {	NULL,	".iifeq",	S_CONDITIONAL,	0,	O_IIFEQ	},
    {	NULL,	".iifne",	S_CONDITIONAL,	0,	O_IIFNE	},
    {	NULL,	".iifb",	S_CONDITIONAL,	0,	O_IIFB	},
    {	NULL,	".iifnb",	S_CONDITIONAL,	0,	O_IIFNB	},
    {	NULL,	".iifidn",	S_CONDITIONAL,	0,	O_IIFIDN},
    {	NULL,	".iifdif",	S_CONDITIONAL,	0,	O_IIFDIF},
    {	NULL,	".else",	S_CONDITIONAL,	0,	O_ELSE	},
    {	NULL,	".endif",	S_CONDITIONAL,	0,	O_ENDIF	},
    {	NULL,	".list",	S_LISTING,	0,	O_LIST	},
    {	NULL,	".nlist",	S_LISTING,	0,	O_NLIST	},
    {	NULL,	".equ",		S_EQU,		0,	O_EQU	},
    {	NULL,	".gblequ",	S_EQU,		0,	O_GBLEQU},
    {	NULL,	".lclequ",	S_EQU,		0,	O_LCLEQU},
    {	NULL,	".byte",	S_DATA,		0,	O_1BYTE	},
    {	NULL,	".db",		S_DATA,		0,	O_1BYTE	},
    {	NULL,	".fcb",		S_DATA,		0,	O_1BYTE	},
    {	NULL,	".word",	S_DATA,		0,	O_2BYTE	},
    {	NULL,	".dw",		S_DATA,		0,	O_2BYTE	},
    {	NULL,	".fdb",		S_DATA,		0,	O_2BYTE	},
/*    {	NULL,	".3byte",	S_DATA,		0,	O_3BYTE	},	*/
/*    {	NULL,	".triple",	S_DATA,		0,	O_3BYTE	},	*/
/*    {	NULL,	".4byte",	S_DATA,		0,	O_4BYTE	},	*/
/*    {	NULL,	".quad",	S_DATA,		0,	O_4BYTE	},	*/
    {   NULL,   ".df",          S_FLOAT,        0,      0       },
    {	NULL,	".blkb",	S_BLK,		0,	O_1BYTE	},
    {	NULL,	".ds",		S_BLK,		0,	O_1BYTE	},
    {	NULL,	".rmb",		S_BLK,		0,	O_1BYTE	},
    {	NULL,	".rs",		S_BLK,		0,	O_1BYTE	},
    {	NULL,	".blkw",	S_BLK,		0,	O_2BYTE	},
/*    {	NULL,	".blk3",	S_BLK,		0,	O_3BYTE	},	*/
/*    {	NULL,	".blk4",	S_BLK,		0,	O_4BYTE	},	*/
    {	NULL,	".ascii",	S_ASCIX,	0,	O_ASCII	},
    {	NULL,	".ascis",	S_ASCIX,	0,	O_ASCIS	},
    {	NULL,	".asciz",	S_ASCIX,	0,	O_ASCIZ	},
    {	NULL,	".str",		S_ASCIX,	0,	O_ASCII	},
    {	NULL,	".strs",	S_ASCIX,	0,	O_ASCIS	},
    {	NULL,	".strz",	S_ASCIX,	0,	O_ASCIZ	},
    {	NULL,	".fcc",		S_ASCIX,	0,	O_ASCII	},
    {	NULL,	".define",	S_DEFINE,	0,	O_DEF	},
    {	NULL,	".undefine",	S_DEFINE,	0,	O_UNDEF	},
    {	NULL,	".even",	S_BOUNDARY,	0,	O_EVEN	},
    {	NULL,	".odd",		S_BOUNDARY,	0,	O_ODD	},
    {	NULL,	".bndry",	S_BOUNDARY,	0,	O_BNDRY	},
    {	NULL,	".msg"	,	S_MSG,		0,	0	},
    {	NULL,	".assume",	S_ERROR,	0,	O_ASSUME},
    {	NULL,	".error",	S_ERROR,	0,	O_ERROR	},
/* sdas specific */
    {   NULL,   ".optsdcc",     S_OPTSDCC,      0,      0       },
/* end sdas specific */

	/* Macro Processor */

    {	NULL,	".macro",	S_MACRO,	0,	O_MACRO	},
    {	NULL,	".endm",	S_MACRO,	0,	O_ENDM	},
    {	NULL,	".mexit",	S_MACRO,	0,	O_MEXIT	},

    {	NULL,	".narg",	S_MACRO,	0,	O_NARG	},
    {	NULL,	".nchr",	S_MACRO,	0,	O_NCHR	},
    {	NULL,	".ntyp",	S_MACRO,	0,	O_NTYP	},

    {	NULL,	".irp",		S_MACRO,	0,	O_IRP	},
    {	NULL,	".irpc",	S_MACRO,	0,	O_IRPC	},
    {	NULL,	".rept",	S_MACRO,	0,	O_REPT	},

    {	NULL,	".nval",	S_MACRO,	0,	O_NVAL	},

    {	NULL,	".mdelete",	S_MACRO,	0,	O_MDEL	},

	/* Machines: only Rabbit CPUs */

    {   NULL,   ".r2k",         S_CPU,          0,      T_R2K   },
    {   NULL,   ".r3ka",        S_CPU,          0,      T_R3KA  },
    {   NULL,   ".r4k00",       S_CPU,          0,      T_R4K00 },
    {   NULL,   ".r4k01",       S_CPU,          0,      T_R4K01 },
    {   NULL,   ".r4k10",       S_CPU,          0,      T_R4K10 },
    {   NULL,   ".r4k11",       S_CPU,          0,      T_R4K11 },
    {   NULL,   ".r4k",         S_CPU,          0,      T_R4K11 },
    {   NULL,   ".r6k00",       S_CPU,          0,      T_R6K00 },
    {   NULL,   ".r6k01",       S_CPU,          0,      T_R6K01 },
    {   NULL,   ".r6k10",       S_CPU,          0,      T_R6K10 },
    {   NULL,   ".r6k11",       S_CPU,          0,      T_R6K11 },

        /* Rabbit instructions */

    {   NULL,   "ld",           S_LD,           0,      0x40    },

    {	NULL,	"call",		S_CALL,		0,	0xCD	},
    {	NULL,	"jp",		S_JP,		0,	0xC2	},
    {	NULL,	"jr",		S_JR,		0,	0x18	},
    {   NULL,   "djnz",         S_DJNZ,         0,      0x10    },
    {   NULL,   "dwjnz",        S_DJNZ,         0,      0xed    },

    {	NULL,	"ret",		S_RET,		0,	0xC0	},

    {   NULL,   "bit",          S_BIT,          0,      0x40    },
    {   NULL,   "res",          S_BIT,          0,      0x80    },
    {   NULL,   "set",          S_BIT,          0,      0xC0    },

    {   NULL,   "inc",          S_INCDEC,       0,      0x04    },
    {   NULL,   "dec",          S_INCDEC,       0,      0x05    },

    {   NULL,   "add",          S_ADD,          0,      0x80    },
    {   NULL,   "adc",          S_ADC,          0,      0x88    },
    {   NULL,   "sbc",          S_SBC,          0,      0x98    },

    {   NULL,   "and",          S_AND,          0,      0xA0    },
    {   NULL,   "or",           S_AND,          0,      0xB0    },
    {   NULL,   "xor",          S_AND,          0,      0xA8    },
    {   NULL,   "cp",           S_AND,          0,      0xB8    },

    {   NULL,   "sub",          S_SUB,          0,      0x90    },

    {   NULL,   "ex",           S_EX,           0,      0xE3    },

    {	NULL,	"push",		S_PUSH,		0,	0xC5	},
    {   NULL,   "pop",          S_PUSH,         0,      0xC1    },

    {   NULL,   "ioi",          S_INH1,          0,     0xD3    },
    {   NULL,   "ioe",          S_INH1,          0,     0xDB    },

    {   NULL,   "rla",          S_RL,           0,      0x17    },
    {   NULL,   "rra",          S_RL,           0,      0x1F    },

    {   NULL,   "rlc",          S_RL,           0,      0x00    },
    {   NULL,   "rrc",          S_RL,           0,      0x08    },
    {   NULL,   "rl",           S_RL,           0,      0x10    },
    {   NULL,   "rr",           S_RL,           0,      0x18    },
    {   NULL,   "sla",          S_RL,           0,      0x20    },
    {   NULL,   "sra",          S_RL,           0,      0x28    },
    {   NULL,   "sll",          S_RL,           0,      0x30    }, 
    {   NULL,   "srl",          S_RL,           0,      0x38    },

    {	NULL,	"rst",		S_RST,		0,	0xC7	},

    {   NULL,   "ccf",          S_INH1,         0,      0x3F    },
    {   NULL,   "cpl",          S_INH1,         0,      0x2F    },

    {   NULL,   "ipset",	S_IM,		0,	0xED	},
    {   NULL,   "ipset0",       S_ED_0ARGS,     0,      0x46    },
    {   NULL,   "ipset1",       S_ED_0ARGS,     0,      0x56    },
    {   NULL,   "ipset2",       S_ED_0ARGS,     0,      0x4E    },
    {   NULL,   "ipset3",       S_ED_0ARGS,     0,      0x5E    },
    {   NULL,   "ipres",        S_ED_0ARGS,     0,      0x5D    },
    {	NULL,	"exx",		S_INH1,		0,	0xD9	},
    {	NULL,	"nop",		S_INH1,		0,	0x00	},

    {   NULL,   "altd",         S_INH1,         0,      0x76    },

    {   NULL,   "rlca",         S_INH1,         0,      0x07    },
    {   NULL,   "rrca",         S_INH1,         0,      0x0F    },
    {   NULL,   "scf",          S_INH1,         0,      0x37    },

    {   NULL,   "ldd",          S_ED_0ARGS,     0,      0xA8    },
    {   NULL,   "lddr",         S_ED_0ARGS,     0,      0xB8    },
    {   NULL,   "ldi",          S_ED_0ARGS,     0,      0xA0    },
    {   NULL,   "ldir",         S_ED_0ARGS,     0,      0xB0    },
    {   NULL,   "neg",          S_NEG,          0,      0x44    },
    {	NULL,	"reti",		S_ED_0ARGS,	0,	0x4D	},
    {   NULL,   "lret",         S_ED_0ARGS,     0,      0x45    },

    {   NULL,   "mul",          S_MUL,          0,      0xF7    },

    {   NULL,   "idet",         R3K_INH1,       0,      0x5B    },
    {   NULL,   "lddsr",        R3K_INH2,       0,      0x98    },
    {   NULL,   "ldisr",        R3K_INH2,       0,      0x90    },
    {   NULL,   "lsddr",        R3K_INH2,       0,      0xD8    },
    {   NULL,   "lsdr",         R3K_INH2,       0,      0xF8    },
    {   NULL,   "lsidr",        R3K_INH2,       0,      0xD0    },
    {   NULL,   "lsir",         R3K_INH2,       0,      0xF0    },
    {   NULL,   "rdmode",       R3K_INH2,       0,      0x7F    },
    {   NULL,   "setusr",       R3K_INH2,       0,      0x6F    },
    {   NULL,   "sures",        R3K_INH2,       0,      0x7D    },
    {   NULL,   "syscall",      R3K_INH2,       0,      0x75    },
    {   NULL,   "uma",          R3K_INH2,       0,      0xC0    },
    {   NULL,   "ums",          R3K_INH2,       0,      0xC8    },

    {   NULL,   "swap",         X_SWAP,         0,      0x00    },

    {   NULL,   "mulu",         X_MULU,         0,      0x00    },
    {   NULL,   "jre",          X_JRE,          0,      0xA3    },
    {   NULL,   "flag",		X_FLAG,		0,	0xA4	},
    {   NULL,   "clr",          X_CLR,          0,      0xBF    },

    {   NULL,   "rlb",          X_RLB_RRB,      0,      0x6F    },
    {   NULL,   "rrb",          X_RLB_RRB,      0,      0x7F    },

    {   NULL,   "ljp",          X_LJP,          0,      0xC7    },
    {   NULL,   "lcall",        X_LJP,          0,      0xCF    },
    {   NULL,   "lljp",         X_LJP,          0,      0x87    },
    {   NULL,   "llcall",       X_LJP,          0,      0x8F    },

    {	NULL,	"setsysp",	S_CALL,		0,	0xB1	},
    {	NULL,	"setusrp",	S_CALL,		0,	0xB5	},

    {   NULL,   "lret",         S_ED_0ARGS,     0,      0x45    },
    {   NULL,   "bool",         X_BOOL,         0,      0xCC    },
    {   NULL,   "ldp",          X_LDP,          0,      0x64    },

    {   NULL,   "sysret",       R4K_INH2,       0,      0x83    },
    {   NULL,   "llret",        R4K_INH2,       0,      0x8B    },
    {   NULL,   "copy",         R4K_INH2,       0,      0x80    },
    {   NULL,   "copyr",        R4K_INH2,       0,      0x88    },
    {   NULL,   "exp",          R4K_INH2,       0,      0xD9    },
    {   NULL,   "fsyscall",     R4K_INH2,       0,      0x55    },

    {   NULL,   "sbox",         X_BOX,          0,      0x02    },
    {   NULL,   "ibox",         X_BOX,          0,      0x12    },

    {   NULL,   "test",         X_TEST,         0,      0x4C    },
    {   NULL,   "cbm",          X_CBM,          0,      0x82    },
    {   NULL,   "alts",         R6K_1_ALW,      0,      0x40    },
    {   NULL,   "altsd",        R6K_1_ALW,      0,      0x64    },

    {   NULL,   "ldf",          X_LDF,          S_EOL,  0x00    },
};
