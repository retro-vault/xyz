/* rab.h */

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

/*)BUILD
	$(PROGRAM) =	ASRAB
	$(INCLUDE) = {
		ASXXXX.H
		RAB.H
	}
	$(FILES) = {
		RABMCH.C
		RABADR.C
		RABPST.C
		ASMAIN.C
		ASDBG.C
		ASLEX.C
		ASSYM.C
		ASSUBR.C
		ASEXPR.C
		ASDATA.C
		ASLIST.C
		ASOUT.C
	}
	$(STACK) = 3000
*/


/*
 * Indirect Addressing delimeters
 */
#define	LFIND		'('
#define RTIND		')'


/*
 * Registers
 */
#define B		0
#define C		1
#define D		2
#define E		3
#define H		4
#define L		5
#define A		7

#define BC		0
#define DE		1
#define HL		2
#define SP		3
#define AF		4
#define IX		5
#define IY		6

#define PW		0
#define PX		1
#define PY		2
#define PZ		3

#define XPC		0
#define LXPC		1

#define EIR             0x47
#define IIR             0x4f
#define IP              0x76

#define BCDE            1
#define JKHL            1

/*
 * Conditional definitions
 */
#define	NZ		0
#define	Z		1
#define	NC		2
#define	CS		3
#define	PO		4
#define	PE		5
#define	P		6
#define	M		7

/*
 * Alternate set of conditional definitions for some rabbit 4000 instructions
 */
#define CC_GT           0
#define CC_GTU          1
#define CC_LT           2
#define CC_V            3
#define CC_NZ           4
#define CC_Z            5
#define CC_NC           6
#define CC_C            7
#define CC_GE         0x8
#define CC_LEU        0x9
#define CC_LE         0xa




/*
 * Symbol types
 */
#define	S_IMMED		30
#define	S_R8		31
#define	S_R8X		32

#define	S_R16		34
#define S_R16_ALT       35
#define	S_CND		36
#define	S_FLAG		37

#define S_R32_BCDE      38
#define S_R32_JKHL      39
#define S_RXPC          40

#define S_R16AF         41
#define S_R16AF_ALT     42

#define S_R8IP	        43
#define S_R16_JK_OR_ALT	44

#define S_R16SU	        45

#define S_R32_PWXYZ	46


/*
 * Indexing modes
 */
#define	S_INDR		50
#define	S_IDBC		50
#define	S_IDDE		51
#define	S_IDHL		52
#define	S_IDSP		53
#define	S_IDIX		55
#define	S_IDIY		56
#define	S_INDM		57

#define S_IDHL_OFFSET   58

#define	S_IDIX_A	59
#define	S_IDIY_A	60

#define	S_IDSP_HL	61
#define	S_IDJKHL	62
#define	S_IDHTR_HL	63

#define	S_INDR_32	70
#define	S_IDPW		70
#define	S_IDPX		71
#define	S_IDPY		72
#define	S_IDPZ		73

#define	S_INDR32_HL	80
#define	S_IDPW_HL	80
#define	S_IDPX_HL	81
#define	S_IDPY_HL	82
#define	S_IDPZ_HL	83

#define	S_INDR32_BC	90
#define	S_IDPW_BC	90
#define	S_IDPX_BC	91
#define	S_IDPY_BC	92
#define	S_IDPZ_BC	93



/*
 * Instruction types
 */
#define	S_LD		60
#define	S_CALL		61
#define	S_JP		62
#define	S_JR		63
#define	S_RET		64
#define	S_BIT		65
#define	S_INCDEC	66
#define	S_ADD		67
#define	S_ADC		68
#define	S_AND		69
#define	S_EX		70
#define	S_PUSH		71
#define	S_RL		72
#define	S_RST		73
#define	S_IM		74
#define	S_INH1		75
#define	S_ED_0ARGS	76
#define	S_DJNZ		77
#define	S_SUB		78
#define	S_SBC		79
#define S_NEG           80
#define	S_CPU		81
#define	S_MUL		82
/* Rabbit specific Instructions */
#define	X_MULU		90
#define X_LJP           91
#define X_BOOL          92
#define X_LDP           93
#define X_R3K_MODE      94
#define R3K_INH1        95
#define R3K_INH2        96
/* the remaining instructions are Rabbit >= 4000 */
#define X_R4K_XFIRST    97
#define X_R4K_MULU	97
#define X_JRE		98
#define X_CLR		99
#define R4K_INH2	100
#define X_TEST		101
#define X_CBM		102
#define X_LDF		103
#define X_SWAP		104
#define R6K_1_ALW       105
#define X_FLAG		106
#define X_BOX		107
#define X_RLB_RRB	108


#define R_2K       0
#define R_3KA      3
#define R_4K       4
#define R_6K       6

#define R_NOMODE   0
#define R_MODE00   1
#define R_MODE01   2
#define R_MODE10   3
#define R_MODE11   4

#define IS_MIN_MODE_01(x) ((x.mode)>=R_MODE01)
#define IS_MODE_10_OR_11(x) ((x.mode)>=R_MODE10)
#define IS_MODE_10(x) ((x.mode)==R_MODE10)
#define IS_MODE_11(x) ((x.mode)==R_MODE11)
#define IS_MIN_3KA(x) ((x.cpu)>=R_3KA)
#define IS_MIN_4K(x) ((x.cpu)>=R_4K)
#define IS_ONLY_4K(x) ((x.cpu)==R_4K)
#define IS_MIN_6K(x) ((x.cpu)>=R_6K)

/*
 * Processor Types (S_CPU)
 */
#define	T_R2K		0
#define	T_R3KA		1
#define T_R4K00         2
#define T_R4K01         3
#define T_R4K10         4
#define T_R4K11         5
#define T_R6K00         6
#define T_R6K01         7
#define T_R6K10         8
#define T_R6K11         9


struct adsym
{
        char    a_str[8];       /* addressing string */
	int	a_val;		/* addressing mode value */
};

        /* register names are in rabadr.c: */
extern	struct	adsym	R8[];
extern	struct	adsym	R8X[];
extern  struct  adsym   R8IP[];
extern	struct	adsym	R16[];
extern  struct  adsym   R16_ALT[];
extern	struct	adsym	R16AF[];
extern  struct  adsym   R16AF_ALT[];

extern  struct  adsym   R32_JKHL[];
extern  struct  adsym   R32_BCDE[];
extern  struct  adsym   RXPC[];

extern	struct	adsym	CND[];
extern  struct  adsym   ALT_CND[];
extern  struct  adsym   R6_CND[];

extern	struct	adsym	R16_JK_OR_ALT[];

extern	struct	adsym	R16SU[];

extern	struct	adsym	R32_PWXYZ[];


	/* machine dependent functions */

        /* rabadr.c */
extern	int		addr(struct expr *esp);
extern	int		admode(struct adsym *sp);
extern  int             any(char c, char *str);
extern	int		srch(char *str);

	/* rabmch.c */
extern	int		genop(int pop, int op, struct expr *esp, int f);
extern	int		gixiy(int v);
extern  int             mchpcr(struct expr *esp);

