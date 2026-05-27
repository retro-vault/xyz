/* asdata.c */

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
 *	jhartman@compuserve.com
 *
 *	Bill McKinnon (BM)
 *	w_mckinnon@conknet.com
 *
 *	Mike McCarty
 *	mike dot mccarty at sbcglobal dot net
 */

#include "asxxxx.h"

/*)Module	asdata.c
 *
 *	The module asdata.c contains the global constants,
 *	structures, and variables used in the assembler.
 */

/*
 *	The number of cycle count digits to display.
 *	The valid range is bounded to be >= 2 and <= 4.
 *	An assembler requiring other than the default
 *	of 2 digits should set this value in minit().
 */
int	cycldgts;

/*
 *	A pointer to the external function
 *	which provides auxiliary functionallity
 *	to the term() function in asexpr.c
 *	returns:	!0 if item processed
 *			 0 if nothing processed
 */
int	(*mchterm_ptr)(struct expr *esp);

/*
 *	A pointer to the external function which
 *	provides auxiliary functionallity to the
 *	.enabl/.dsabl processing in asmain.c
 *	returns:	!0 if item processed
 *			 0 if nothing processed
 */
int	(*mchoptn_ptr)(char *id, int v);


int	aserr;		/*	ASxxxx error counter
			 */
int	trcflags;	/*	ASxxxx tracing flags
			 */
jmp_buf	jump_env;	/*	compiler dependent structure
			 *	used by setjmp() and longjmp()
			 */
/*
 *	Parameters which specify the optional multi-pass
 *	processing for assemblers that have variable
 *	length instructions.
 */
int	nflglmt;	/* command option pass 1 repeat limit
			 */
int	passlmt;	/* default maximum pass 1 repeat limit
			 */ 
int	passcnt;	/* number of passes executed
			 */
int	passJLH;	/* JLH output pass
			 */
int	passfuz;	/* residual fuss after pass == 1
			 */

/*
 *	The asmf structure contains the information
 *	pertaining to an assembler source file/macro.
 *
 * The Parameters:
 *	next	is a pointer to the next object in the linked list
 *	objtyp	specifies the object type - T_ASM, T_INCL, T_MACRO
 *	line	is the saved line number of the parent object
 *	fp	is the source FILE handle
 *	afp	is the file path length (excludes the files name.ext)
 *	afn[]	is the assembler/include file path/name.ext
 *
 *	struct	asmf
 *	{
 *		struct	asmf *next;	Link to Next Object
 *		int	objtyp;		Object Type
 *		int	line;		Saved Line Counter
 *		int	flevel;		saved flevel
 *		int	tlevel;		saved tlevel
 *		int	lnlist;		saved lnlist
 *		FILE *	fp;		FILE Handle
 *		int	afp;		File Path Length
 *		char	afn[FILSPC];	File Name
 *	};
 */
struct	asmf	*asmc;	/*	Pointer to the current
			 *	source input structure
			 */
struct	asmf	*asmo;	/*	The pointer to the first
			 *	command line insert structure
			 */
struct	asmf	*asmp;	/*	The pointer to the first assembler
			 *	source file structure of a linked list
			 */
struct	asmf	*asmi;	/*	Queued pointer to an include file
			 *	source input structure
			 */
struct	asmf	*asmq;	/*	Queued pointer to a macro
			 *	source input structure
			 */

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
 *	darg	is number of macro defintion default values
 *	bgndrg	is a pointer to the first macro defintion default value
 *	enddrg	is a pointer to	the last macro definition default value
 *	xarg	is the number of expansion arguments at macro invocation
 *	bgnxrg	is a pointer to the first expansion argument string
 *	endxrg	is a pointer to the last  expansion argument string
 *
 *	struct	mcrdef {
 *		struct mcrdef *	next;		link to next macro definition
 *		char *		name;		pointer to the macro name
 *		struct strlst *	bgnlst;		link to first text line of macro
 *		struct strlst *	endlst;		link to last text line of macro
 *		int		type;		macro type
 *		int		rptcnt;		repeat counter
 *		int		nest;		macro nesting counter
 *		int		narg;		number of macro defintion arguments
 *		struct strlst * bgnarg;		link to first macro defintion argument
 *		struct strlst * endarg;		link to last macro definition argument
 *		int		darg;		number of macro defintion default values
 *		struct strlst * bgndrg;		link to first macro defintion default value
 *		struct strlst * enddrg;		link to last macro definition default value
 *		int		xarg;		number of macro expansion arguments
 *		struct strlst * bgnxrg;		link to first macro expansion argument
 *		struct strlst * endxrg;		link to last macro xpansion argument
 *	};
 */
struct mcrdef *	mcrlst;	/*	link to list of defined macros
			 */
struct mcrdef *	mcrp;	/*	link to list of defined macros
			 */

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
 *
 *	struct	memlnk {
 *		struct memlnk *	next;		link to next memlnk
 *		void *		ptr;		pointer to allocated memory
 *	};
 */
struct memlnk * asxmem;	/*	Assembler Memory Allocation Structure
			 */
struct memlnk * pmcrmem;/*	First Macro Memory Allocation Structure
			 */
struct memlnk * mcrmem;	/*	Macro Memory Allocation Structure
			 */
int	asmblk;		/*	new data blocks allocated
			 */
int	mcrblk;		/*	new data blocks allocated
			 */
int	incfil;		/*	include file nesting counter
			 */
int	maxinc;		/*	maximum include file nesting encountered
			 */
int	mcrfil;		/*	macro nesting counter
			 */
int	maxmcr;		/*	maximum macro nesting encountered
			 */
int	mcrcnt;		/*	regular macros created
			 */
int	mcrexe;		/*	regular macros executed
			 */
int	inlcnt;		/*	inline macros created
			 */
int	inlexe;		/*	inline macros executed
			 */
int	mlevel;		/*	macro Stack Level
			 */
struct mstack mstk[MAXMCR]; /*	macro Stack
			 */
int	alevel;		/*	area stack pointer
			 */
struct	area *astack[16]; /*	area stack
			 */
int	flevel;		/*	IF-ELSE-ENDIF flag (false != 0)
                         *      the flag will be non zero
                         *      for false conditional case
			 */
int	ftflevel;	/*	IIFF-IIFT-IIFTF FLAG
			 */
int	tlevel;		/*	current conditional level
			 */
int	lnlist;		/*	LIST-NLIST options
			 */
int	ifcnd[MAXIF+1];	/*	array of IF statement condition
			 *	values (0 = FALSE) indexed by tlevel
			 */
int	iflvl[MAXIF+1];	/*	array of IF-ELSE-ENDIF flevel
			 *	values indexed by tlevel
			 */
char	afn[FILSPC];	/*	current input file specification
			 */
int	afp;		/*	current input file path length
			 */
char	afntmp[FILSPC];	/*	temporary input file specification
			 */
int	afptmp;		/*	temporary input file path length
			 */
int	srcline;	/*	current source line number
			 */
int	asmline;	/*	current assembler file line number
			 */
int	incline;	/*	current include file line number
			 */
int	mcrline;	/*	current macro line number
			 */
int	radix;		/*	current number conversion radix:
			 *	2 (binary), 8 (octal), 10 (decimal),
			 *	16 (hexadecimal)
			 */
int	line;		/*	current assembler source
			 *	line number
			 */
int	page;		/*	current page number
			 */
int	lop;		/*	current line number on page
			 */
time_t	curtim;		/*	pointer to the current time string
			 */
int	pass;		/*	assembler pass number
			 */
int	aflag;		/*	-a, make all symbols global flag
			 */
int	bflag;		/*	-b(b), listing modes flag
			 */
int	cflag;		/*	-c, disable cycle counts in listing flag
			 */
int	fflag;		/*	-f(f), relocations flagged flag
			 */
int	gflag;		/*	-g, make undefined symbols global flag
			 */
int	iflag;		/*	-i, insert command line string flag
			 */
int	jflag;		/*	-j, enable NoICE Debug Symbols
			 */
int	kflag;		/*	-k, disable error output to .lst file
			 */
int	lflag;		/*	-l, generate listing flag
			 */
int     nflag;          /*      -n, don't resolve global assigned value symbols flag
			 */
int	oflag;		/*	-o, generate relocatable output flag
			 */
int	pflag;		/*	-p, disable listing pagination
			 */
int	rflag;		/*	-r, line numbers output to .lst to .rst hint file
			 */
int	sflag;		/*	-s, generate symbol table flag
			 */
int	tflag;		/*	-t, output diagnostic parameters from assembler
			 */
int	uflag;		/*	-u, disable .list/.nlist processing flag
			 */
int	vflag;		/*	-v, enable out of range signed / unsigned errors
			 */
int	wflag;		/*	-w, enable wide listing format
			 */
int	xflag;		/*	-x, listing radix flag
			 */
int	yflag;		/*	-y, enable SDCC Debug Symbols
			 */
int	zflag;		/*	-z, disable symbol case sensitivity
			 */
int     waddrmode;      /*      WORD Address mode flag
			 */
int	a_bytes;	/*	REL file T Line address length
			 */
a_uint	a_mask;		/*	Address Mask
			 */
a_uint	s_mask;		/*	Sign Mask
			 */
a_uint	v_mask;		/*	Value Mask
			 */
a_uint	p_mask;		/*	Page Mask
			 */
int	as_msb;		/*	current MSB byte select
			 *	0 == low byte
			 *	1 == high byte
			 *	2 == third byte
			 *	3 == fourth byte
			 */
a_uint	laddr;		/*	address of current assembler line
			 *	or value of .if argument
			 */
a_uint	fuzz;		/*	tracks pass to pass changes in the
			 *	address of symbols caused by
			 *	variable length instruction formats
			 */
int	lmode;		/*	listing mode
			 */
char *	eqt_area;	/*	pointer to the area name
			 *	associated with lmode = ELIST
			 */
char	*ep;		/*	pointer into error list
			 *	array eb[NERR]
			 */
char	eb[NERR];	/*	array of generated error codes
			 */
char	*ex[NERR];	/*	array of error string pointers
			 */
char	*ip;		/*	pointer into the assembler-source
			 *	text line in ib[]
			 */
char    *ib;            /*      assembler-source text line for processing
                         */
char    *ic;            /*      assembler-source text line for listing
                         */
char	*il;		/*	pointer to the assembler-source
			 *	text line to be listed
			 */
char	*cp;		/*	pointer to assembler output
			 *	array cb[]
			 */
char	cb[NCODE];	/*	array of assembler output values
			 */
int	*cpt;		/*	pointer to assembler relocation type
			 *	output array cbt[]
			 */
int	cbt[NCODE];	/*	array of assembler relocation types
			 *	describing the data in cb[]
			 */
int	awg;		/*	default size value (special coding)
			 *	0 - default not defined, 1-4 - 1-4 bytes
			 */
int	csn;		/*	'C' Style Numbers - 0nnn (Octal), 0xnnn (Hex), Else Decimal
			 *	0 - default disabled, 1 - enabled
			 */
int	opcycles;	/*	opcode execution cycles
			 */
char	tb[NTITL];	/*	Title string buffer
			 */
char	stb[NSBTL];	/*	Subtitle string buffer
			 */
char    erb[NINPUT+4];  /*      Error string buffer
			 */

char	symtbl[] = { "Symbol Table" };
char	aretbl[] = { "Area Table" };

char	module[NCPS+2];	/*	module name string
			 */
/* sdas specific */
int     org_cnt;        /*      .org directive counter
			 */
char    *optsdcc;       /*      sdcc compile options
			 */
/* end sdas specific */

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
 *
 *	struct	mne
 *	{
 *		struct	mne *m_mp;	Hash link
 *		char *	m_id;		Mnemonic (JLH)
 *		char	m_type;		Mnemonic subtype
 *		char	m_flag;		Mnemonic flags
 *		a_uint	m_valu;		Value
 *	};
 */
struct	mne	*mnehash[NHASH];

/*
 *	The sym structure is a linked list of symbols defined
 *	in the assembler source files.  The first symbol is "."
 *	defined here.  The entry 'struct tsym *s_tsym'
 *	links any temporary symbols following this symbol and
 *	preceeding the next normal symbol.  The structure also
 *	contains the symbol's name, type (USER or NEW), flag
 *	(global, assigned, and multiply defined), a pointer
 *	to the area structure defining where the symbol is
 *	located, a reference number assigned by outgsd() in
 *	asout.c, and the symbols address relative to the base
 *	address of the area where the symbol is located.
 *
 *	struct	sym
 *	{
 *		struct	sym  *s_sp;	Hash link
 *		struct	tsym *s_tsym;	Temporary symbol link
 *		char	*s_id;		Symbol (JLH)
 *		char	s_type;		Symbol subtype
 *		char	s_flag;		Symbol flags
 *		struct	area *s_area;	Area line, 0 if absolute
 *		int	s_ref;		Ref. number
 *		a_uint	s_addr;		Address
 * sdas specific
 *              a_uint  s_org;          Start Address if absolute
 * end sdas specific
 *	};
 */
struct	sym	sym[] = {
    {	NULL,	NULL,	".",	    S_USER, 0,			NULL,0,0, 0 },
    {	NULL,	NULL,	".__.ABS.", S_USER, S_ASG|S_GBL,	NULL,0,0, 0 },
    {	NULL,	NULL,	".__.CPU.", S_USER, S_ASG|S_LCL,	NULL,0,0, 0 },
    {	NULL,	NULL,	".__.H$L.", S_USER, S_ASG|S_LCL,	NULL,0,0, 0 },
    {	NULL,	NULL,	".__.$$$.", S_USER, S_ASG|S_LCL|S_EOL,	NULL,0,0, 0 }
};

struct	sym	*symp;		/*	pointer to a symbol structure
				 */
struct	sym *symhash[NHASH];	/*	array of pointers to NHASH
				 *	linked symbol lists
				 */

/*
 *	The area structure contains the parameter values for a
 *	specific program or data section.  The area structure
 *	is a linked list of areas.  The initial default area
 *      is "_CODE" defined here, the next area structure
 *	will be linked to this structure through the structure
 *      element 'struct area *a_ap'.  The structure contains the
 *	area name, area reference number ("_CODE" is 0) determined
 *	by the order of .area directives, area size determined
 *	from the total code and/or data in an area, area fuzz is
 *	a variable used to track pass to pass changes in the
 *	area size caused by variable length instruction formats,
 *	and area flags which specify the area's relocation type.
 *
 *	struct	area
 *	{
 *		struct	area *a_ap;	Area link
 *		char *	a_id;		Area Name
 *		int	a_ref;		Reference number
 *		a_uint	a_size;		Area size
 *		a_uint	a_fuzz;		Area fuzz
 *		int	a_flag;		Area flags
 *	};
 */
struct  area    area[] = {
    {NULL,      "_CODE",        0,      0,      0,      A_CON|A_REL}
};

struct  area    *areap; /*      pointer to an area structure
                         */

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
 *	and output data file suffix (appended as a suffix to the
 *	output file name) are optional parameters of the .bank
 *	assembler directive which are passed to the linker as the
 *	default link parameters, and the bank flags which specify
 *	what options have been specified.
 *
 *	struct	bank
 *	{
 *		struct	bank *b_bp;	Bank link
 *		char *	b_id;		Bank Name
 *		char *	b_fsfx;		Bank File Suffix
 *		int	b_ref;		Ref. number
 *		a_uint	b_base;		Bank base address
 *		a_uint	b_size;		Bank length
 *		a_uint	b_map;		Bank mapping
 *		int	b_flag;		Bank flags
 *	};
 *
 *	Pointer to a bank structure
 */
 /*
  * struct	bank	*bankp = &bank[1];
  */

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
 *
 *	struct def
 *	{
 *		struct def	*d_dp;		link to next define
 *		char		*d_id;		defined string
 *		char		*d_define;	string to substitute for defined string
 *		int		d_dflag;	(1) .defined / (0) .undefined
 *	};
 *
 *	Pointer to a def structure
 */
struct	def	*defp = NULL;


FILE	*hfp;		/*	.lst to .rst hint file handle
			 */
FILE	*lfp;		/*	list output file handle
			 */
FILE	*ofp;		/*	relocation output file handle
			 */
FILE	*tfp;		/*	symbol table output file handle
			 */
char	txt[NTXT];	/*	T Line Values
			 */
char	rel[NREL];	/*	R Line Values
			 */
char	*txtp = &txt[0];/*	Pointer to T Line Values
			 */
char	*relp = &rel[0];/*	Pointer to R Line Values
			 */
/*
 * ASCII Table
 */
/*----	\000	\001	\002	\003	\004	\005	\006	\007	*/
/*----------------------------------------------------------------------*/
/*\000	NUL	SOH	STX	ETX	EOT	ENQ	ACK	BELL	*/
/*\010	BS	TAB	LF	VT	FF	CR	SO	SI	*/
/*\020	DLE	DC1	DC2	DC3	DC4	NAK	SYN	ETB	*/
/*\030	CAN	EM	SUB	ESC	FS	GS	RS	US	*/
/*\040	SPACE	!	"	#	$	%	&	`	*/
/*\050	(	)	*	+	`	-	.	/	*/
/*\060	0	1	2	3	4	5	5	7	*/
/*\070	8	9	:	;	<	=	>	?	*/
/*\100	@	A	B	C	D	E	F	G	*/
/*\110	H	I	J	K	L	M	N	O	*/
/*\120	P	Q	R	S	T	U	V	W	*/
/*\130	X	Y	Z	[	\	]	^	_	*/
/*\140	'	a	b	c	d	e	f	g	*/
/*\150	h	i	j	k	l	m	n	o	*/
/*\160	p	q	r	s	t	u	v	w	*/
/*\170	x	y	z	{	|	}	~	DEL	*/

/*
 *	an array of character types,
 *	one per ASCII character
 */
unsigned char   ctype[256] = {
/*NUL*/	ILL,	ILL,	ILL,	ILL,	ILL,	ILL,	ILL,	ILL,
/*BS*/	ILL,	SPACE,	ILL,	ILL,	SPACE,	ILL,	ILL,	ILL,
/*DLE*/	ILL,	ILL,	ILL,	ILL,	ILL,	ILL,	ILL,	ILL,
/*CAN*/	ILL,	ILL,	ILL,	ILL,	ILL,	ILL,	ILL,	ILL,
/*SPC*/	SPACE,	ETC,	ETC,	ETC,	LETTER,	BINOP,	BINOP,	ETC,
/*(*/	ETC,	ETC,	BINOP,	BINOP,	ETC,	BINOP,	LETTER,	BINOP,
/*0*/	DGT2,	DGT2,	DGT8,	DGT8,	DGT8,	DGT8,	DGT8,	DGT8,
/*8*/	DGT10,	DGT10,	ETC,	ETC,	BINOP,	ETC,	BINOP,	ETC,
/*@*/	ETC,	LTR16,	LTR16,	LTR16,	LTR16,	LTR16,	LTR16,	LETTER,
/*H*/	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,
/*P*/	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,
/*X*/	LETTER,	LETTER,	LETTER,	BINOP,	ETC,	ETC,	BINOP,	LETTER,
/*`*/	ETC,	LTR16,	LTR16,	LTR16,	LTR16,	LTR16,	LTR16,	LETTER,
/*h*/	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,
/*p*/	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,	LETTER,
/*x*/	LETTER,	LETTER,	LETTER,	ETC,	BINOP,	ETC,	ETC,	ETC,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER,
        LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER, LETTER
};

/*
 *	an array of characters which
 *	perform the case translation function
 */
char	ccase[256] = {
/*NUL*/	'\000',	'\001',	'\002',	'\003',	'\004',	'\005',	'\006',	'\007',
/*BS*/	'\010',	'\011',	'\012',	'\013',	'\014',	'\015',	'\016',	'\017',
/*DLE*/	'\020',	'\021',	'\022',	'\023',	'\024',	'\025',	'\026',	'\027',
/*CAN*/	'\030',	'\031',	'\032',	'\033',	'\034',	'\035',	'\036',	'\037',
/*SPC*/	'\040',	'\041',	'\042',	'\043',	'\044',	'\045',	'\046',	'\047',
/*(*/	'\050',	'\051',	'\052',	'\053',	'\054',	'\055',	'\056',	'\057',
/*0*/	'\060',	'\061',	'\062',	'\063',	'\064',	'\065',	'\066',	'\067',
/*8*/	'\070',	'\071',	'\072',	'\073',	'\074',	'\075',	'\076',	'\077',
/*@*/	'\100',	'\141',	'\142',	'\143',	'\144',	'\145',	'\146',	'\147',
/*H*/	'\150',	'\151',	'\152',	'\153',	'\154',	'\155',	'\156',	'\157',
/*P*/	'\160',	'\161',	'\162',	'\163',	'\164',	'\165',	'\166',	'\167',
/*X*/	'\170',	'\171',	'\172',	'\133',	'\134',	'\135',	'\136',	'\137',
/*`*/	'\140',	'\141',	'\142',	'\143',	'\144',	'\145',	'\146',	'\147',
/*h*/	'\150',	'\151',	'\152',	'\153',	'\154',	'\155',	'\156',	'\157',
/*p*/	'\160',	'\161',	'\162',	'\163',	'\164',	'\165',	'\166',	'\167',
/*x*/	'\170',	'\171',	'\172',	'\173',	'\174',	'\175',	'\176',	'\177',
        '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
        '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
        '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
        '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
        '\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
        '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
        '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
        '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
        '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
        '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
        '\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
        '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
        '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
        '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
        '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
        '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377'
};

