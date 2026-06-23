/* rabmch.c */

/*
 *  Copyright (C) 1989-2009  Alan R. Baldwin
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
 * xerr messages Copyright (C) 1989-2021  Alan R. Baldwin
 * from ASxxxx 5.40
 */

/*
 * Extensions: P. Felber
 * Altered by Leland Morrison to support rabbit 2000 
 *   and rabbit 4000 instruction sets (2011)
 * More Rabbit 4000 and 6000 instructions and modes:
 *   Janko Stamenović 2026
 */

#include "asxxxx.h"
#include "rab.h"

char    *cpu    = "Rabbit 2000/4000/6000";
char	*dsft	= "asm";

char	imtab[3] = { 0x46, 0x56, 0x5E };

static const unsigned char ipset[4] = { 0x46, 0x56, 0x4E, 0x5E };

struct rabbit {
	unsigned int cpu;
	unsigned int mode;
} rab;


/*
 * Opcode Cycle Definitions
 */
#define	OPCY_SDP	((char) (0xFF))
#define	OPCY_ERR	((char) (0xFE))

/*      OPCY_NONE       ((char) (0x80)) */
/*      OPCY_MASK       ((char) (0x7F)) */

#define	OPCY_CPU	((char) (0xFD))

#define	UN	((char) (OPCY_NONE | 0x00))
#define	P2	((char) (OPCY_NONE | 0x01))
#define	P3	((char) (OPCY_NONE | 0x02))
#define	P4	((char) (OPCY_NONE | 0x03))
#define	P5	((char) (OPCY_NONE | 0x04))
#define	P6	((char) (OPCY_NONE | 0x05))
#define	P7	((char) (OPCY_NONE | 0x06))

/*
 * Process a machine op.
 */
void
machine(struct mne *mp)
{
	int op, t1, t2;
	struct expr e1, e2;
	int rf, v1, v2;
        struct expr *ep;

	clrexpr(&e1);
	clrexpr(&e2);
	op = (int) mp->m_valu;
	rf = mp->m_type;

        if (rab.cpu < R_4K && rf >= X_R4K_XFIRST)
                rf = 0;

	switch (rf) {
	case S_CPU:
		switch (op) {
		case T_R2K:   rab.cpu=R_2K;  rab.mode = R_NOMODE; break;
		case T_R3KA:  rab.cpu=R_3KA; rab.mode = R_NOMODE; break;
		case T_R4K00: rab.cpu=R_4K;  rab.mode = R_MODE00; break;
		case T_R4K01: rab.cpu=R_4K;  rab.mode = R_MODE01; break;
		case T_R4K10: rab.cpu=R_4K;  rab.mode = R_MODE10; break;
		case T_R4K11: rab.cpu=R_4K;  rab.mode = R_MODE11; break;
		case T_R6K00: rab.cpu=R_6K;  rab.mode = R_MODE00; break;
		case T_R6K01: rab.cpu=R_6K;  rab.mode = R_MODE01; break;
		case T_R6K10: rab.cpu=R_6K;  rab.mode = R_MODE10; break;
		case T_R6K11: rab.cpu=R_6K;  rab.mode = R_MODE11; break;
		default: break;
		}
		sym[2].s_addr = op;
		opcycles = OPCY_CPU;
		lmode = SLIST;
		break;

	case S_INH1:
		outab(op);
		break;

	case S_ED_0ARGS:
		outab(0xED);
		outab(op);
		break;

	case S_RET:
		if (more()) {
			if ((v1 = admode(CND)) != 0) {
				outab(op | (v1<<3));
			} else {
				xerr('a', "Condition code required.");
			}
		} else {
			outab(0xC9);
		}
		break;

	case S_PUSH:
		t1 = addr(&e1);
		v1 = (int) e1.e_addr;
		if (t1 == S_USER)
			t1 = e1.e_mode = S_IMMED;
		if (t1 == S_R16AF) {
			outab(op+0x30);
			break;
                }
		if (t1 == S_R8IP) {
                        outab(0xED);
                        if (op == 0xC5)
                                outab(0x76);  /* push */
                        else
                                outab(0x7E);  /* pop  */
                        break;
		}
		if (t1 == S_R16SU && IS_MIN_3KA(rab)) {
                        outab(0xED);
                        if (op == 0xC5)
                                outab(0x66);  /* push */
                        else
                                outab(0x6E);  /* pop  */
                        break;
		}
		if (t1 == S_R16 && (v1 &= 0xFF) != SP) {
			if (v1 != gixiy(v1)) {
				outab(op+0x20);
				break;
			}
			outab(op | (v1<<4));
			break;
                }
		if (IS_MIN_4K(rab)) {
                        if (t1 == S_R32_BCDE || t1 == S_R32_JKHL) {
				outab((t1 == S_R32_BCDE) ? 0xDD : 0xFD);
                                outab(op+0x30);
                                break;
                        }
			if ( t1 == S_IMMED ) {
				outab(0xED);
				outab(0xA5);
				outrw(&e1, 0);
				break;
			}
		}
		xerr('a', "Invalid Addressing Mode.");
		break;

	case S_RST:
	        v1 = (int) absexpr();
                /* Rabbit RSTs: only 10 18 20 28 38
                 */
                if (v1 == 0x00 || v1 == 0x08 || v1 == 0x30) {
                        xerr('a', "Rabbit CPUs: 0x00, 0x08, and 0x30 don't exist");
                        break;
                }
		if (v1 & ~0x38) {
			xerr('a', "Allowed values: N * 0x08, N = 0 -> 7."); 
                        break;
		}
		outab(op|v1);
		break;

        /* Rabbit processor use the opcode to set interrupt level */
	case S_IM:
                /* ipset 0-3 */
		expr(&e1, 0);
		abscheck(&e1);
                if (e1.e_addr > 3) {
                        xerr('a', "Values of 0, 1, 2, and 3 are valid.");
			break;
		}
		outab(op);
                outab(ipset[e1.e_addr]);
		break;

	case S_BIT:
		expr(&e1, 0);
		t1 = 0;
		v1 = (int) e1.e_addr;
		if (v1 > 7) {
			v1 &= 0x07;
			++t1;
		}
		op |= (v1<<3);
		comma(1);
                addr(&e2);
		abscheck(&e1);
		if (genop(0xCB, op, &e2, 0) || t1)
			xerr('a', "Invalid Addressing Mode.");
		break;

	case S_RL:
		if (op == 0x17 || op == 0x1F) { // rla rra
			if (!more()) {
				outab(op);
				break;
			}
			t1 = addr(&e1);
			v1 = (int)e1.e_addr;
			if (t1 == S_USER)
				t1 = e1.e_mode = S_IMMED;
			if (IS_MIN_4K(rab) && t1 == S_IMMED && v1 == 8) {
				comma(1);
				t2 = addr(&e2);
				if (t2 == S_R32_BCDE || t2 == S_R32_JKHL) {
					outab((t2 == S_R32_BCDE) ? 0xDD : 0xFD);
					outab((op == 0x17) ? 0x6F : 0x7F);
					break;
				}
			}
			aerr( );
		}
		t1 = 0;
		t2 = addr(&e2);
		if (t2 == S_USER)
			t2 = e1.e_mode = S_IMMED;
                if (t2 == S_IMMED && IS_MIN_4K(rab)) {
                        v1 = (int) e2.e_addr;
                        /* v1 should be shift count of 1,2,4, or 8 */
                        comma(1);
                        clrexpr(&e2);
                        t2 = addr(&e2);

                        if ((t2 != S_R32_BCDE) && (t2 != S_R32_JKHL))
                                aerr( );
                        if (v1 == 1)
                                v1 = 0x48;
                        else if (v1 == 2)
                                v1 = 0x49;
                        else if (v1 == 4)
                                v1 = 0x4B;
                        else if ((v1 == 8) && (op < 0x20 /* rlc, rrc, rl, rr */))
                                v1 = 0x4F;
                        else {
                                err('o');
                                break;
                        }
			outab((t2 == S_R32_BCDE) ? 0xDD : 0xFD);
                        /* op:
			   00 rlc  08 rrc     10 rl   18 rr     
                         * 20 sla  28 sra     30 sll  38 srl */
                        outab(v1 + (op << 1));
                        break;
                }
                else if (more()) {
                        if ((t2 != S_R8) || (e2.e_addr != A))
                                ++t1;
                        comma(1);
                        clrexpr(&e2);
                        t2 = addr(&e2);
                }
		else if (t2 == S_R16) {
                        v2 = (int) e2.e_addr;
                        if ((v2 == DE) && 
                            ((op == 0x10 /* rl */) || (op == 0x18 /* rr */))) {
                                outab( 0xF3 - 0x10 + op );
                                break;
                        }

                        if ((v2 == HL || v2 == IX || v2 == IY) && 
				op == 0x18 /* rr */ ) {
				gixiy(v2);
                                outab( 0xFC );
                                break;
                        }

                        if (IS_MODE_10_OR_11(rab)) {
                                if ((v2 == HL) && (op == 0x10 /* rl */)) {
					if (IS_MODE_10(rab))
						outab( 0x7F );
                                        outab( 0x42 );
                                        break;
                                }
                                if (((v2 == BC)||(v2 == DE)) &&
                                    (op < 0x20 /* rlc, rrc, rl, rr */)) {
					if (IS_MODE_10(rab))
						outab( 0x7F );
                                        outab( 0x50 + (op >> 3) + ((v2==BC)?0x10:0x00) );
                                        break;
                                }
                        }

                        aerr( );
		}
		if (op == 0x30) /* sll doesn't exist here */
			op = 0x20;
		if (genop(0xCB, op, &e2, 0) || t1)
                        aerr();
		break;

        case S_AND:  /* and, xor, or, cp */
        case S_SUB:  /* sub */
        case S_SBC:  /* sbc */
		t1 = addr(&e1);
		if (t1 == S_USER)
			t1 = e1.e_mode = S_IMMED;
                if (!(more())) {
                        /* handle case for implicit target of 'A' register */
                        t2 = t1;
			e2.e_mode = e1.e_mode;
                        t1 = S_R8;
                        v1 = A;
                        v2 = (int) e1.e_addr;
                        ep = &e1;
		} else {
			comma(1);
			t2 = addr(&e2);
			if (t2 == S_USER)
				t2 = e2.e_mode = S_IMMED;
			v1 = (int) e1.e_addr;
			v2 = (int) e2.e_addr;
                        ep = &e2;
		}

		if ((t1 == S_R8) && (v1 == A)) {
                        if ( ((t2 == S_R8) && (v2 == A)) &&
                             ((op == 0xA8) || (op == 0xB0)) ) {
                                /* AF: "xor a,a"  ||  B7: "or a,a" */
                                outab( op | 0x07 );
                                break;
                        }

                        if ((t2 == S_R8) || (t2 == S_IDHL)) {
                                /* ljm - rabbit 4000 support
                                 * (sub,sbc,and,xor,or,cp) A,R  or A,(HL)
                                 * needs a 0x7F prefix byte when
                                 * operating in rabbit 4000 mode
                                 */
                                if (IS_MODE_11(rab))
                                        outab(0x7F);
                        }

#if 0
                        if (t2 == S_IMMED) {  /* AND,XOR,OR,CP,SUB A, #n */
                                /* opcode for (sub,sbc,and,xor,or,cp) A,#immediate 
                                 * do not need 0x7F prefix byte
                                 */
                                outab(op|0x46);  /* 0xA0 | 0x46 => 0xE6, etc */
                                outrb(ep, 0);
			} else
#endif

                        if (genop(0, op, ep, 1))
                                aerr();
                        break;
                }

                if ((t1 == S_R16) && (v1 == HL) &&
                    (t2 == S_R16) && (rf == S_SBC)) {
                        /* sbc  hl, [bc|de|hl|sp] */
                        if ( v2 != gixiy(v2) )
                                /* sorry, sbc hl, [ix|iy] do not exist */
                                xerr('a', "Second argument: must be BC, DE, HL or SP.");

                        outab(0xED);
                        outab(0x42 | v2 << 4);
                        break;
                }
		if (t1 == S_R16 && v1 == HL &&
		     rf == S_SUB && t2 == S_R16_JK_OR_ALT
		     && (v2&0xff) == 0 && IS_MODE_10_OR_11(rab)) {
			if (IS_MODE_10(rab))
				outab( 0x7F );
			outab(0x45);
			break;
		}
                if ((t1 == S_R16) && (v1 == HL) &&
                    (t2 == S_R16) && (v2 == DE)) {
			if (op == 0xA0) { /* and */
				outab(0xDC); break;
			}
			if (op == 0xB0) { /* or */
				outab(0xEC); break;
			}
			if (IS_MIN_4K(rab)) {
				if (rf == S_SBC) /* no sbc hl, de */
					xerr('a', "Not valid for SBC.");
				if (op == 0xB8) { 
					outab( 0xED ); /* cp  hl, de */
					outab( 0x48 );
					break;
				}
			}
			if (IS_MODE_10_OR_11(rab)) {
                                if (!IS_MODE_11(rab))
                                        outab(0x7F);
				if (op == 0x90) {
					outab(0x55); /* sub hl, de */ 
					break;
				} else if (op == 0xA8) {
					outab(0x54); /* xor hl, de*/ 
					break;
				}
			}
                }
		if (t1 == S_R16 && v1 == HL && t2 == S_IDSP 
			&& IS_MIN_6K(rab) && IS_MIN_MODE_01(rab)) {
			outab(0x49);
                        switch( op ) {
                        case 0x90:  /* sub */ op = 0xAA; break;
                        case 0x98:  /* sbc */ op = 0xBA; break;
                        case 0xA0:  /* and */ op = 0xCA; break;
                        case 0xA8:  /* xor */ op = 0xDA; break;
                        case 0xB0:  /* or  */ op = 0xEA; break;
                        case 0xB8:  /* cp  */ op = 0xFA; break;
                        }	
			outab(op);
			outrb(&e2, 0);
			break;
		}
		if (t1 == S_R16 && v1 == HL && 
			(t2 == S_IDIX || t2 == S_IDIY) &&
			IS_MIN_6K(rab)) {
			outab((t2 == S_IDIX)? 0xDD : 0xFD);
                        switch( op ) {
                        case 0x90:  /* sub */ op = 0x90; break;
                        case 0x98:  /* sbc */ op = 0x91; break;
                        case 0xA0:  /* and */ op = 0xA0; break;
                        case 0xA8:  /* xor */ op = 0xA1; break;
                        case 0xB0:  /* or  */ op = 0xB0; break;
                        case 0xB8:  /* cp  */ op = 0xB1; break;
                        }	
			outab(op);
			outrb(&e2, 0);
			break;
		}
		if (op == 0xB8 && t1 == S_R16 && v1 == HL && 
			t2 == S_IMMED && IS_MODE_10_OR_11(rab)) {
                                if (!IS_MODE_11(rab))
                                        outab(0x7F);
				outab(0x48);
                                outrb(&e2, 0);
                                break;
		}
                if ((t1 == S_R16) && ((v1 == IX) || (v1 == IY)) &&
                    (t2 == S_R16) && (v2 == DE) &&
                    ((op == 0xA0 /* and */) || (op == 0xB0 /* or */))) {
                        v1 = gixiy(v1);
                        outab(op + 0x3C);
                        break;
                }

                if ((t1 == S_R32_JKHL) && (t2 == S_R32_BCDE)) {
                        /* ED D6   sub  jkhl, bcde  */
                        /*         sbc  jkhl, bcde does not exist */
                        /* ED E6   and  jkhl, bcde  */
                        /* ED EE   xor  jkhl, bcde  */
                        /* ED F6   or   jkhl, bcde  */
                        /* ED 58   cp   jkhl, bcde  */
                        if (rf == S_SBC) /* op == 0x98 */
                                xerr('a', "Not valid for SBC.");

                        outab(0xED);
                        switch( op ) {
                        case 0x90:  /* sub */ outab(0xD6); break;
                        case 0xA0:  /* and */ outab(0xE6); break;
                        case 0xA8:  /* xor */ outab(0xEE); break;
                        case 0xB0:  /* or  */ outab(0xF6); break;
                        case 0xB8:  /* cp  */ outab(0x58); break;
                        }
                        break;
                }

                xerr('a', "Invalid Addressing Mode.");
                break;

	case S_ADD:
	case S_ADC:
		t1 = addr(&e1);
		if (t1 == S_USER)
			t1 = e1.e_mode = S_IMMED;
		t2 = 0;
		if (more()) {
			comma(1);
			t2 = addr(&e2);
			if (t2 == S_USER)
				t2 = e2.e_mode = S_IMMED;
		}
		if (t2 == 0) {
                        /* implied destination of the 8-bit 'a' register */
                        if ((t1 == S_R8) || (t1 == S_IDHL)) {
                                /* ljm - rabbit 4000 support
                                 * (add,adc,sub,sbc,and,xor,or,cp) A,R  or A,(HL)
                                 * needs a 0x7F prefix byte when
                                 * operating in rabbit 4000 mode
                                 */
                                if (IS_MODE_11(rab))
                                        outab(0x7F);
                        }

			if (genop(0, op, &e1, 1))
				xerr('a', "Invalid Addressing Mode.");
			break;
		}
		if ((t1 == S_R8) && (e1.e_addr == A)) {
                        if ( ((t2 == S_R8) || (t2 == S_IDHL)) && IS_MODE_11(rab) )
                                /* ljm - rabbit 4000 support, see note in t2==0 */
                                outab(0x7F);

			if (genop(0, op, &e2, 1))
				xerr('a', "Second argument: Invalid Addressing Mode.");
			break;
		}

                if (t1 == S_R32_JKHL && t2 == S_R32_BCDE &&
                        rf == S_ADD && IS_MIN_4K(rab)) {
                        /* rabbit 4000 - ED C6   "add  jkhl, bcde"  */
                        outab(0xED);
                        outab(0xC6);
                        break;
                }

		v1 = (int) e1.e_addr;
		v2 = (int) e2.e_addr;

                if (t1 == S_R16 && v1 == SP && t2 == S_IMMED) {
                        /* add sp,#n  n=signed displacement */
			outab(0x27);
			outrb(&e2, 0);
			break;
		}

                if (t1 == S_R16 && v1 == HL) {
			if (t2 == S_R16) {
				if (v2 > SP)
					aerr( );

				if (rf == S_ADC)
					outab(0xED);

				op = (rf == S_ADD) ? 0x09 : 0x4A;
				outab(op | (v2 << 4));
				break;
			}
			if (t2 == S_R16_JK_OR_ALT && 
				(v2&0xff) == 0 &&
				IS_MODE_10_OR_11(rab)) {
				if (IS_MODE_10(rab))
					outab( 0x7F );
				outab(0x65); // add hl, jk
				break;
			}
                }

		if (t1 == S_R16 && (v1 == IX || v1 == IY)) {
			if (t2 == S_R16) {
				if ((v2 == HL) ||
				    (((v2 == IX) || (v2 == IY)) && (v2 != v1)))
					aerr( );

				if ((v2 == IX) || (v2 == IY))
					v2 = HL;

				gixiy(v1);
				outab(0x09 | (v2 << 4));
				break;
			}
			if (t2 == S_IMMED && rf == S_ADD && 
			    IS_MIN_6K(rab)) {
				gixiy(v1);
				outab(0xC5);
				outrb(&e2, 0);
				break;
			}
                }

		if (t1 == S_R16 && v1 == HL && t2 == S_IDSP 
			&& IS_MIN_6K(rab) && IS_MIN_MODE_01(rab)) {
			outab(0x49);
			outab((op == 0x80) ? 0x8A : 0x9A);
			outrb(&e2, 0);
			break;
		}
		if (t1 == S_R16 && v1 == HL && 
			(t2 == S_IDIX || t2 == S_IDIY) &&
			IS_MIN_6K(rab)) {
			outab((t2 == S_IDIX)? 0xDD : 0xFD);
			outab((op == 0x80) ? 0x80 : 0x81);
			outrb(&e2, 0);
			break;
		}

		xerr('a', "Invalid Addressing Mode.");
		break;

	case S_LD:
		t1 = addr(&e1);
		if (t1 == S_USER)
			t1 = e1.e_mode = S_IMMED;
		v1 = (int) e1.e_addr;
		comma(1);
		t2 = addr(&e2);
		if (t2 == S_USER)
			t2 = e1.e_mode = S_IMMED;
		v2 = (int) e2.e_addr;
                if (t1 == S_R8) {
                        if (t2 == S_IMMED) {
                                outab((e1.e_addr<<3) | 0x06);
                                outrb(&e2, 0);
                                break;
                        }

                        if (IS_MODE_10_OR_11(rab) 
				&& (v1 == A) && (t2 == S_R8) && (v2 == A)) {
                                /* exception for "ld a,a" 
                                 * on rabbit 4000 0x7F is a prefix instead of "ld a,a"
                                 */
                                xerr('a', "Not A Rabbit 4000 Instruction");
                        }

                        if ((v1 == A) && (t2 == S_R8)) {
                                /* "ld  a,r", (except "ld a,a") */
                                v1 = op | e1.e_addr<<3;
                                if (genop(0, v1, &e2, 0))
                                        aerr( );
                                break;
                        }

                        /* ld [b,c,d,e,h,l,a], _ */
                        if ((t2 == S_R8) && (v2 != A)) {
                                /* 8-bit register to 8-bit register */
                                /* use 0x7F prefix when in rabbit 4000 mode */
                                v1 = op | e1.e_addr<<3;
                                if (IS_MODE_11(rab))
                                        outab(0x7F);
                                if (genop(0, v1, &e2, 0) == 0)
                                        break;

                                aerr( );
                        }

                        if ((t2 == S_R8) && (v2 == A) &&
                            ((v1 != A) || (!(IS_MODE_11(rab))))) {
                                /* "ld  r,a", but except "ld a,a" 
                                 * on rabbit 4000 0x7F is a prefix instead of "ld a,a" */
                                v1 = op | e1.e_addr<<3;
                                if (genop(0, v1, &e2, 0))
                                        aerr( );
                                break;
                        }

                        if ((t2 == S_IDHL) || (t2 == S_IDIX) || (t2 == S_IDIY)) {
                                /* "ld r,(hl)" or "ld r,disp (ix|iy)" */
                                v1 = op | e1.e_addr<<3;
                                if (genop(0, v1, &e2, 0))
                                        aerr( );
                                break;
                        }
                }
      
		if ((t1 == S_R16) && (t2 == S_IMMED)) {
                        v1 = gixiy(v1);  /* generayes prefix when ix or iy */
			outab(0x01|(v1<<4));
			outrw(&e2, 0);
			break;
		}
		if ((t1 == S_R16) && (t2 == S_INDM)) {
			if (gixiy(v1) == HL) {
				outab(0x2A);
			} else {
				outab(0xED);
				outab(0x4B | (v1<<4));
			}
			outrw(&e2, 0);
			break;
		}
		if (t1 == S_R16_JK_OR_ALT && (v1&0xff)==0
			&& IS_MODE_10_OR_11(rab)) {
				if (t2 == S_INDM) {
					if (IS_MODE_10(rab))
						outab( 0x7F );
					outab(0x99);
					outrw(&e2, 0);
					break;
				} else if (t2 == S_IMMED) {
					if (IS_MODE_10(rab))
						outab( 0x7F );
					outab(0xA9);
					outrw(&e2, 0);
					break;
				}
		}
                if ((t1 == S_R16) && (v1 == HL)) {
                        if ((t2 == S_IDIX) || (t2 == S_IDIY) ||
                            (t2 == S_IDHL) || (t2 == S_IDHL_OFFSET)) {
                                /* LD HL,n(IX|HL|IY) */
                                if (t2 == S_IDIY)
                                        outab(0xFD);
                                else if ((t2 == S_IDHL) || (t2 == S_IDHL_OFFSET))
                                        outab(0xDD); /* LD HL,n(HL) */

                                outab(0xE4);
                                outrb(&e2, 0);
                                break;
                        }
                        if ((t2 == S_R16) && ((v2 == IX) || (v2 == IY))) {
                                outab((v2==IX) ? 0xDD : 0xFD);
                                outab(0x7C);
                                break;
                        }
                        if (IS_MODE_10_OR_11(rab)) {
                                if ((t2 == S_R16) && ((v2 == BC) || (v2 == DE))) {
					if (IS_MODE_10(rab))
						outab( 0x7F );
                                        outab( 0x81 + ((v2 == DE) ? 0x20 : 0) );
					break;
				}
			}
		}
                if ((t2 == S_R16) && (v2 == HL)) /* ld n(IX|IY|HL), HL */
                {
                        if ((t1 == S_IDIY) || (t1 == S_IDHL) ||
                            (t1 == S_IDHL_OFFSET))
                                outab((t1==S_IDIY) ? 0xFD : 0xDD);

                        if ((t1 == S_IDIY) || (t1 == S_IDIX) ||
                            (t1 == S_IDHL) || (t1 == S_IDHL_OFFSET)) {
                                outab(0xF4);
                                outrb(&e1, 0);
                                break;
                        }

                        if ((t1 == S_R16) && ((v1 == IX) || (v1 == IY))) {
                                outab((v1==IX) ? 0xDD : 0xFD);
                                outab(0x7D);
				break;
			}
		}
		if ((t1 == S_INDM) && (t2 == S_R16)) {
			if (gixiy(v2) == HL) {
				outab(0x22);
			} else {
				outab(0xED);
				outab(0x43 | (v2<<4));
			}
			outrw(&e1, 0);
			break;
		}
		if (t1 == S_INDM && t2 == S_R16_JK_OR_ALT
			&& (v2&0xff)==0
			&& IS_MODE_10_OR_11(rab)) {
			if (IS_MODE_10(rab))
				outab( 0x7F );
			outab(0x89);
			outrw(&e1, 0);
			break;
		}
		if ((t1 == S_R8) && (v1 == A) && (t2 == S_INDM)) {
			outab(0x3A);
			outrw(&e2, 0);
			break;
		}
		if ((t1 == S_INDM) && (t2 == S_R8) && (v2 == A)) {
			outab(0x32);
			outrw(&e1, 0);
			break;
		}
		if ((t2 == S_R8) && (gixiy(t1) == S_IDHL)) {
			outab(0x70|v2);
			if (t1 != S_IDHL)
                                outrb(&e1, 0);
			break;
		}
		if ((t2 == S_IMMED) && (gixiy(t1) == S_IDHL)) {
			outab(0x36);
			if (t1 != S_IDHL)
                                outrb(&e1, 0);
			outrb(&e2, 0);
			break;
		}
		if ((t1 == S_R8X) && (t2 == S_R8) && (v2 == A)) {
			outab(0xED);
			outab(v1);
			break;
		}
		if ((t1 == S_R8) && (v1 == A) && (t2 == S_R8X)) {
			outab(0xED);
			outab(v2|0x10);
			break;
		}
		if ((t1 == S_R16) && (v1 == SP)) {
			if ((t2 == S_R16) && (gixiy(v2) == HL)) {
				outab(0xF9);
				break;
			}
		}
                if ((t1 == S_R16) && (t2 == S_IDSP))
                {
                        if ( (v1=gixiy(v1)) == HL ) {
                                /* ljm - added rabbit instruction:
                                 * LD HL|IX|IY, n(SP)
                                 */
                                outab(0xC4);
                                outrb(&e2, 0);
                                break;
                        }
                }
      
                if ((t1 == S_IDSP) && (t2 == S_R16))
                {
                        //printf( "at %s: %d, t1=%d, v1=%d, t2=%d, v2=%d\n",
                        //  __FILE__, __LINE__, t1, v1, t2, v2 );
                        if ( (v2=gixiy(v2)) == HL ) {
                                /* ljm - added rabbit instruction:
                                 * LD HL|IX|IY, n(SP)
                                 */
                                outab(0xD4);
                                outrb(&e1, 0);
                                break;
                        }
                }
		if ((t1 == S_R8) && (v1 == A)) {
			if ((t2 == S_IDBC) || (t2 == S_IDDE)) {
				outab(0x0A | ((t2-S_INDR)<<4));
				break;
			}
		}
		if ((t2 == S_R8) && (v2 == A)) {
			if ((t1 == S_IDBC) || (t1 == S_IDDE)) {
				outab(0x02 | ((t1-S_INDR)<<4));
				break;
			}
		}
      
                /* load/save code bank register "xpc" */
                if (t1 == S_RXPC && v1 == XPC && t2 == S_R8 && v2 == A) {
                        outab(0xED);
                        outab(0x67);
                        break;
                }
      
                if (t1 == S_RXPC && v1 == XPC && IS_MODE_10_OR_11(rab) &&
                    t2 == S_R16 && v2 == HL) {
			if (IS_MODE_10(rab))
				outab( 0x7F );
                        outab(0x97);
                        break;
                }
      
                if (t2 == S_RXPC && v2 == XPC && t1 == S_R8 && v1 == A) {
                        outab(0xED);
                        outab(0x77);
                        break;
                }
      
                if (t2 == S_RXPC && v2 == XPC && IS_MODE_10_OR_11(rab) &&
                    t1 == S_R16 && v1 == HL) {
			if (IS_MODE_10(rab))
				outab( 0x7F );
                        outab(0x9F);
                        break;
                }

                if ((t1 == S_R16_ALT) && (t2 == S_R16)) {
                        if ((v2 == BC) || (v2 == DE)) {
                                /* LD BC'|DE'|HL', BC|DE */
                                outab(0xED);
                                outab(((v2 == BC) ? 0x49 : 0x41) | (v1 << 4));
                                break;
                        }
                }
      
                /* 16-bit operations valid only in rabbit 4000 mode */
                if (IS_MODE_10_OR_11(rab) && (t1 == S_R16) && (t2 == S_R16)) {
                        if ((v1 == HL) && ((v2 == BC) || (v2 == DE))) {
				if (IS_MODE_10(rab))
					outab( 0x7F );
                                outab( 0x81 + ((v2==DE)?0x20:0x00) );
                                break;
                        }
                        if ((v2 == HL) && ((v1 == BC) || (v1 == DE))) {
				if (IS_MODE_10(rab))
					outab( 0x7F );
                                outab( 0x91 + ((v1==DE)?0x20:0x00) );
                                break;
                        }
                }
      
                /* 32-bit operations valid in rabbit 4000 mode */
                if (IS_MIN_4K(rab) && ((t1 == S_R32_JKHL) || (t1 == S_R32_BCDE))) {
                        if (t2 == S_IDHL) {
                                outab((t1 == S_R32_JKHL) ? 0xFD : 0xDD);
                                outab( 0x1A );
                                break;
                        }
                        if ((t2 == S_IDIX) || (t2 == S_IDIY) || (t2 == S_IDSP)) {
                                outab((t1 == S_R32_JKHL) ? 0xFD : 0xDD);
                                if (t2 == S_IDSP)
                                        v2 = 0x20;
                                else 
                                        v2 = ((t2 == S_IDIY) ? 0x10 : 0x00);
          
                                outab( 0xCE + v2 );
                                outrb(&e2, 0);
                                break;
                        }
                        if (t2 == S_INDM) {
				if (IS_MODE_10(rab))
					outab( 0x7F );
                                outab( 0x93 + ((t1 == S_R32_JKHL) ? 1 : 0) );
                                outrw(&e2, 0);
                                break;
                        }
                        if (t2 == S_IMMED) {
				if (IS_MODE_10(rab))
					outab( 0x7F );
                                outab( 0xA3 + ((t1 == S_R32_JKHL) ? 1 : 0) );
                                outrb(&e2, 0);
                                break;
                        }
                }

                if (IS_MIN_4K(rab) && ((t2 == S_R32_JKHL) || (t2 == S_R32_BCDE))) {
                        if (t1 == S_IDHL) {
                                outab((t2 == S_R32_JKHL) ? 0xFD : 0xDD);
                                outab( 0x1B );
                                break;
                        }
                        if ((t1 == S_IDIX) || (t1 == S_IDIY) || (t1 == S_IDSP)) {
                                outab((t2 == S_R32_JKHL) ? 0xFD : 0xDD);
                                if (t1 == S_IDSP)
                                        v1 = 0x20;
                                else
                                        v1 = ((t1 == S_IDIY) ? 0x10 : 0x00);

                                outab( 0xCF + v1 );
                                outrb(&e1, 0);
                                break;
                        }
                        if (t1 == S_INDM && IS_MODE_10_OR_11(rab)) {
				if (IS_MODE_10(rab))
					outab( 0x7F );
                                outab( 0x83 + ((t2 == S_R32_JKHL) ? 1 : 0) );
                                outrw(&e1, 0);
				break;
			}
		}
		xerr('a', "Invalid Addressing Mode.");
		break;

     
	case S_EX:
		t1 = addr(&e1);
		comma(1);
		t2 = addr(&e2);
		if (t2 == S_R16) {
			v1 = (int) e1.e_addr;
			v2 = (int) e2.e_addr;
                        if (t1 == S_R16) {
                                if ((v1 == DE) && (v2 == HL)) {
                                        outab(0xEB);
                                        break;
                                }
                                if (IS_MODE_10_OR_11(rab) && (v1==BC) && (v2==HL)) {
					if (IS_MODE_10(rab))
						outab( 0x7F );
                                        outab(0xB3);
                                        break;
                                }
                        }
                        else if (t1 == S_R16_ALT) {
                                if ((v1 == DE) && (v2 == HL)) {
                                        /* EX DE', HL */
                                        outab(0xE3);
                                        break;
                                }
                                if (IS_MIN_4K(rab) && (v1==BC) && (v2==HL)) {
                                        /* EX BC', HL */
                                        outab(0xED);
                                        outab(0x74);
                                        break;
                                }
			} else if (IS_MODE_10_OR_11(rab)) {
				if (t1 == S_R16_JK_OR_ALT && v2 == HL) {
					if (v1) {
						outab( 0xED );
						outab( 0x7C );
						break;
					} else {
						if (IS_MODE_10(rab))
							outab( 0x7F );
						outab( 0xB9 );
						break;
					}
				}
			}
                        if ((t1 == S_IDSP) && (v1 == 0)) {
                                /* 0xE3 is EX DE',HL on rabbit 2000 
                                 * but DD/FD E3 "ex (sp),ix|iy" is valid
                                 */
                                if (v2 == HL) {
                                        outab(0xED);
                                        outab(0x54);
                                        break;
                                }
                                else if (gixiy(v2) == HL) {
                                        outab(op);
                                        break;
                                }
                        }
                }
                if ((t1 == S_R16AF) && (t2 == S_R16AF_ALT)) {
                        outab(0x08);
                        break;
                }
                if (t1==S_R32_JKHL && t2==S_R32_BCDE &&
			IS_MODE_10_OR_11(rab)) {
			if (IS_MODE_10(rab))
				outab( 0x7F );
                        outab(0xB4);
                        break;
                }
		xerr('a', "Invalid Addressing Mode.");
		break;
            
	case S_INCDEC:
		t1 = addr(&e1);
		v1 = (int) e1.e_addr;
		if (t1 == S_R8) {
			outab(op|(v1<<3));
			break;
		}
		if (t1 == S_IDHL) {
			outab(op|0x30);
			break;
		}
		if (t1 != gixiy(t1)) {
			outab(op|0x30);
			outrb(&e1, 0);
			break;
		}
		if (t1 == S_R16 && (op == 0x04 || op == 0x05)) {
			v1 = gixiy(v1);
			op = (op == 0x04) ? 0x03 : 0x0B;
			outab(op | (v1<<4));
			break;
		}
		xerr('a', "Invalid Addressing Mode.");
		break;
      
        case S_NEG:
                if (!more()) {
                        /* "neg" implies "neg a" */
			outab(0xED);
			outab(op);
			break;
		}
		t1 = addr(&e1);
		v1 = (int) e1.e_addr;
                if (t1 == S_R8 && v1 == A) { /* "neg a" */
			outab(0xED);
			outab(op);
			break;
		}

                if ((t1 == S_R32_JKHL || t1 == S_R32_BCDE) && 
			IS_MIN_4K(rab)) {
                        /* neg jkhl|bcde */
                        outab((t1 == S_R32_BCDE) ? 0xDD : 0xFD);
                        outab(0x4D);
                        break;
                }
      
                if (t1 == S_R16 && v1 == HL
			&& IS_MODE_10_OR_11(rab)) {
			/* "neg hl" */
			if (IS_MODE_10(rab))
				outab( 0x7F );
                        outab(0x4D);
                        break;
		}
		xerr('a', "Invalid Addressing Mode.");
		break;
      
	case S_DJNZ:
	case S_JR:
		if (IS_MIN_6K(rab) && (v1 = admode(R6_CND)) != 0) {
			op = 0x80 | ((v1)&0x3)<<3;
			if (IS_MODE_10(rab))
				outab( 0x7F );
			comma(1);
		}
                else if ((v1 = admode(CND)) != 0 && rf != S_DJNZ) {
			v1 &= 0xFF;
			if (v1 <= 0x03) {
				op += (v1+1)<<3;
			} else {
				xerr('a', "Condition code required.");
			}
			comma(1);
		}
		else if (IS_MODE_10_OR_11(rab) &&
			((v1 = admode(ALT_CND)) != 0) &&
			((v1&0xFF) < CC_NZ)) {
			op = 0xA0 | (v1&0xFF)<<3;
			if (IS_MODE_10(rab))
				outab( 0x7F );
			comma(1);
		} else if (IS_MIN_4K(rab) && op == 0xED) { 
			outab(0xED); /* dwjnz */
			op = 0x10; 
		}
		expr(&e2, 0);
		outab(op);
                if (mchpcr(&e2)) {
                        v2 = (int) (e2.e_addr - dot.s_addr - 1);
                        if (pass == 2 && ((v2 < -128) || (v2 > 127)))
				xerr('a', "Branching Range Exceeded.");
			outab(v2);
		} else {
			outrb(&e2, R_PCR);
		}
		if (e2.e_mode != S_USER)
			rerr();
		break;
      
	case S_CALL:
		t1 = addr(&e1);
		if (op == 0xCD) { /* call */
			if (IS_MIN_4K(rab)) {
				if (t1 == S_IDHL || t1 == S_IDIX || t1 == S_IDIY) {
					outab((t1 == S_IDHL) ? 0xED 
						: ((t1 == S_IDIX) ? 0xDD : 0xFD));
					outab(0xEA);
					break;
				}
			}
		} else { /* setsysp setusrp */
			if (IS_MIN_4K(rab))
				outab(0xED);
		}
		outab(op);
		outrw(&e1, 0);
		break;
      
	case S_JP:
		if (IS_MIN_6K(rab) && (v1 = admode(R6_CND)) != 0) {
			op = 0x43 | (((v1)&0x3)<<3);
			comma(1);
			expr(&e1, 0);
                        if (IS_MODE_10(rab))
                                outab(0x7F);
			outab(op);
			outrw(&e1, 0);
			break;
		}
		if ((v1 = admode(CND)) != 0) {
			op |= (v1&0xFF)<<3;
			comma(1);
			expr(&e1, 0);
			outab(op);
			outrw(&e1, 0);
			break;
		}
		if (IS_MODE_10_OR_11(rab) &&
			((v1 = admode(ALT_CND)) != 0) &&
			((v1&0xFF) < CC_NZ)) {
			op = 0xA2 | ((v1&0xFF)<<3);
			comma(1);
			expr(&e1, 0);
			if (IS_MODE_10(rab))
				outab( 0x7F );
			outab(op);
			outrw(&e1, 0);
			break;
		}
		t1 = addr(&e1);
		if (t1 == S_USER) {
			outab(0xC3);
			outrw(&e1, 0);
			break;
		}
                if ((e1.e_addr == 0) && (gixiy(t1) == S_IDHL)) {
			outab(0xE9);
			break;
		}

		xerr('a', "Invalid Addressing Mode.");
		break;
     
        case X_LJP: 
		if (op == 0xC7 || op == 0xCF) { /* ljp lcall */
			t1 = addr(&e1);
			if (t1 == S_USER)
				t1 = e1.e_mode = S_IMMED;
			comma(1);
			t2 = addr(&e2);
			if (t2 == S_USER)
				t2 = e2.e_mode = S_IMMED;
			if (t1 == S_IMMED && t2 == S_IMMED) {
				outab(op);
				outrw(&e2, 0);
				outrb(&e1, 0);
				break;
			}
		}
		if (op == 0x87) { /* lljp */
			if ((v1 = admode(ALT_CND)) != 0) {
				comma(1);
				t1 = addr(&e1);
				if (t1 == S_USER)
					t1 = e1.e_mode = S_IMMED;
				comma(1);
				t2 = addr(&e2);
				if (t2 == S_USER)
					t2 = e2.e_mode = S_IMMED;
				outab(0xED);
				outab(0xA2 + ((v1&0xff) << 3));
				outrw(&e2, 0);
				outrw(&e1, 0);
				break;
			}
		}
		if (op == 0x87 || op == 0x8F) { /* lljp llcall */
			t1 = addr(&e1);
			if (t1 == S_USER)
				t1 = e1.e_mode = S_IMMED;
			comma(1);
			t2 = addr(&e2);
			if (t2 == S_USER)
				t2 = e2.e_mode = S_IMMED;
			if (IS_MODE_10(rab))
				outab( 0x7F );
			outab(op);
			outrw(&e2, 0);
			outrw(&e1, 0);
			break;
		}
		aerr( );
		break;

	case X_FLAG:
		if (IS_MIN_4K(rab) && ((v1 = admode(ALT_CND)) != 0)) {
			op = 0xA4 + ((v1&0xFF)<<3);
			comma(1);
			t2 = addr(&e2);
			v2 = (int) e2.e_addr;
			if (t2 == S_R16 && v2 == HL) {
				outab(0xED);
				outab(op);
				break;
			}
		}
		if (IS_MIN_6K(rab) && ((v1 = admode(R6_CND)) != 0)) {
			v1 &= 0xFF;
			op = ((v1 == CC_LEU) ? 0x9C : (0xE4 | (v1 << 3)));
			comma(1);
			t2 = addr(&e2);
			v2 = (int) e2.e_addr;
			if (t2 == S_R16 && v2 == HL) {
				outab(0xED);
				outab(op);
				break;
			}
		}
		xerr('a', "Invalid Addressing Mode.");
		break;
      
        case X_BOOL:
		t1 = addr(&e1);
		v1 = (int) e1.e_addr;
                if ((t1 == S_R16) && ((v1 == HL) || (v1 == IX) || (v1 == IY))) {
			v1 = gixiy(v1);
			outab(op);
			break;
		}
		xerr('a', "Invalid Addressing Mode.");
		break;

        case X_LDP:
		t1 = addr(&e1);
		v1 = (int) e1.e_addr;
		comma(1);
		t2 = addr(&e2);
		v2 = (int) e2.e_addr;
                /* LDP (mn), HL|IX|IY */
		if ((t1 == S_INDM) && (t2 == S_R16)) {
			if ((v2 != HL) && (v2 != IX) && (v2 != IY)) {
				xerr('a', "Second argument: must be HL, IX, or IY.");
				break;
			}
                        if (v2 == HL) {
				outab(0xED);
			} else {
				gixiy(v2);
			}
                        outab(op | 0x01);
			outrw(&e1, 0);
			break;
		}
                /* LDP HL|IX|IY, (mn) */
		if ((t1 == S_R16) && (t2 == S_INDM)) {
			if ((v1 != HL) && (v1 != IX) && (v1 != IY)) {
				xerr('a', "First argument: must be HL, IX, or IY.");
				break;
			}
			if (v1 == HL) {
				outab(0xED);
			} else {
				gixiy(v1);
			}
                        outab(op | 0x09);
			outrw(&e2, 0);
			break;
		}
                /* LDP (HL|IX|IY), HL */
		if ((t2 == S_R16) && (v2 == HL)) {
			if ((t1 != S_IDHL) && (t1 != S_IDIX) && (t1 != S_IDIY)) {
				xerr('a', "First argument: must be (HL), (IX), or (IY).");
				break;
			}
			if ((e1.e_base.e_ap != NULL) || (v1 != 0)) {
				xerr('a', "First argument: (HL+D, (IX+D), and (IY+D) are invalid.");
				break;
			}
			if (t1 == S_IDHL) {
				outab(0xED);
			} else {
				gixiy(t1);
			}
			outab(op);
			break;
		}
                /* LDP HL, (HL|IX|IY) */
		if ((t1 == S_R16) && (v1 == HL)) {
			if ((t2 != S_IDHL) && (t2 != S_IDIX) && (t2 != S_IDIY)) {
				xerr('a', "Second argument: must be (HL), (IX), or (IY).");
				break;
			}
			if ((e2.e_base.e_ap != NULL) || (v2 != 0)) {
				xerr('a', "Second argument: (HL+D, (IX+D), and (IY+D) are invalid.");
				break;
			}
			if (t2 == S_IDHL) {
				outab(0xED);
			} else {
				gixiy(t2);
			}
                        outab(op | 0x08);
			break;
		}
		xerr('a', "Invalid Addressing Mode.");
		break;
	case X_LDF:
		if (IS_MIN_4K(rab)) {
			t1 = addr(&e1);
			v1 = (int) e1.e_addr;
			comma(1);
			t2 = addr(&e2);
			v2 = (int) e2.e_addr;
			op = 0;
			if (t1 == S_INDM && t2 == S_R8 && v2 == A) {
				op = 0x8A;
				ep = &e1;
			}
			if (t1 == S_R8 && v1 == A && t2 == S_INDM) {
				op = 0x9A;
				ep = &e2;
			}
			if (t1 == S_INDM && t2 == S_R16 && v2 == HL) {
				op = 0x82;
				ep = &e1;
			}
			if (t1 == S_R16 && v1 == HL && t2 == S_INDM) {
				op = 0x92;
				ep = &e2;
			}
			if (t1 == S_INDM && t2 == S_R16 && 
				(v2 == BC || v2 == DE || v2 == IX || v2 == IY)) {
				outab(0xED);
				if (v2 == IX) v2 = 2;
				if (v2 == IY) v2 = 3;
				op = 0x0B + (v2 << 4);
				ep = &e1;
				outab(op);
				outr3b(ep,0);
				break;
			}
			if (t1 == S_R16 && t2 == S_INDM &&
				(v1 == BC || v1 == DE || v1 == IX || v1 == IY)) {
				outab(0xED);
				if (v1 == IX) v1 = 2;
				if (v1 == IY) v1 = 3;
				op = 0x0A + (v1 << 4);
				ep = &e2;
				outab(op);
				outr3b(ep,0);
				break;
			}
			if (op > 0) {
				if (IS_MODE_10(rab))
					outab( 0x7F );
				outab(op);
				outr3b(ep,0);
				break;
			}
			if (t1 == S_R32_BCDE || t1 == S_R32_JKHL && t2 == S_INDM) {
				op = 0x0A;
				ep = &e2;
			}
			if (t1 == S_INDM && t2 == S_R32_BCDE || t2 == S_R32_JKHL) {
				op = 0x0B;
				ep = &e1;
				t1 = t2;
			}
			if (op > 0) {
				outab((t1 == S_R32_BCDE) ? 0xDD : 0xFD);
				outab(op);
				outr3b(ep,0);
				break;
			}
		}
		xerr('a', "Invalid Addressing Mode.");
		break;


        case R3K_INH1:
                if (!IS_MIN_3KA(rab))
                        xerr('o', "A Rabbit 3000A/4000 Instruction.");
                outab(op);
                break;
      
        case R3K_INH2:
                if (!IS_MIN_3KA(rab))
                        xerr('o', "A Rabbit 3000A/4000 Instruction.");
                outab(0xED);
                outab(op);
                break;

        case R4K_INH2:
                if (!IS_MIN_4K(rab))
                        xerr('o', "A Rabbit 4000 Instruction.");
                outab(0xED);
                outab(op);
                break;

	case R6K_1_ALW:
                if (!IS_MIN_6K(rab))
                        xerr('o', "A Rabbit 6000 Instruction.");
                outab(op);
                break;
      
	case S_MUL:
	case X_MULU:
		if (!more()) {
			if (rf == S_MUL) {
				outab(op);
				break;
			}
			if (rf == X_MULU && IS_MIN_4K(rab)) {
				// r4k mulu
				if (IS_MODE_10(rab))
					outab( 0x7F );
				outab(0xA7);
				break;
			}
		}
		if (IS_MIN_6K(rab)) {
			t1 = addr(&e1);
			v1 = (int) e1.e_addr;
			comma(1);
			t2 = addr(&e2);
			v2 = (int) e2.e_addr;
			if (t1 == S_R16 && v1 == HL &&
				t2 == S_R16 && v2 == DE &&
				IS_MODE_10_OR_11(rab)) {
				if (IS_MODE_10(rab))
					outab( 0x7F );
				outab((rf == S_MUL) ? 0x59 : 0x69);
				break;
			}
		}
		xerr('a', "Invalid Addressing Mode.");
		break;
      
        case X_JRE:
                if (!IS_MODE_10_OR_11(rab))
                        xerr('o', "A Rabbit 4000 Instruction.");
                if ((v1 = admode(ALT_CND)) != 0) {
                        op += v1<<3;
			comma(1);

		} else {
                        op = 0x98;
		}
		expr(&e2, 0);
		if (op == 0x98 && IS_MODE_10(rab))
			outab( 0x7F );
		if (op != 0x98) {
			outab( 0xED );
		}
		outab(op);
                if (mchpcr(&e2)) {
                        v2 = (int) (e2.e_addr - dot.s_addr - 2);
                        if (pass == 2 && ((v2 < -32768) || (v2 > 32767)))
                                aerr();
                        outab( (v2 & 0xFF) );
                        outab( (v2 >> 8) );
		} else {
			rerr();
		}
		if (e2.e_mode != S_USER)
			rerr();
		break;
      
        case X_CLR:
                if (!(IS_MODE_10_OR_11(rab)))
                        xerr('o', "A Rabbit 4000 Instruction.");
		t1 = addr(&e1);
		v1 = (int) e1.e_addr;
		if ((t1 == S_R16) && (v1 == HL)) {
			if (IS_MODE_10(rab))
				outab( 0x7F );
			outab(op);
			break;
		}
                aerr( );
		break;

	case X_TEST:
                if (!IS_MIN_4K(rab))
                        xerr('o', "A Rabbit 4000 Instruction.");
		t1 = addr(&e1);
		v1 = (int) e1.e_addr;
		if (t1 == S_R16) {
			if (v1 == BC) {
				outab( 0xED );
				outab(0x4C);
				break;
			}
			if (v1 == HL) {
				if (IS_MODE_10(rab))
					outab( 0x7F );
				outab(0x4C);
				break;
			}
			if (v1 == IX || v1 == IY) {
				gixiy(v1);
				outab(0x4C);
				break;
			}
		} else if (t1 == S_R32_BCDE) {
			outab(0xDD);
			outab(0x5C);
			break;
		} else if (t1 == S_R32_JKHL) {
			outab(0xFD);
			outab(0x5C);
			break;
		}

		aerr( );
		break;

	case X_CBM:
		if (!IS_MIN_4K(rab))
			xerr('o', "A Rabbit 4000 Instruction.");
		t1 = addr(&e1);
		if (t1 == S_USER)
			t1 = e1.e_mode = S_IMMED;
		v1 = (int) e1.e_addr;
		if (t1 == S_IMMED) {
			outab(0xED);
			outab(0x00);
			outrb(&e1, 0);
			break;
		}
		aerr( );
		break;

	case X_SWAP:
		if (IS_MIN_6K(rab)) {
			t1 = addr(&e1);
			v1 = (int) e1.e_addr;
			if (t1 == S_R8) {
				outab(0xED);
				outab(0x87 + (v1<<4));
				break;
			} else if (t1 == S_R16) {
				outab(0xED);
				outab(0xCF + (v1<<4));
				break;
			} else if (t1 == S_R16_JK_OR_ALT && v1 == 0) {
				outab(0xED);
				outab(0xFF);
				break;
			} else if (t1 == S_R32_BCDE || t1 == S_R32_JKHL) {
				outab((t1 == S_R32_BCDE) ? 0xDD : 0xFD);
				outab(0x32);
				break;
			}
		}
		aerr( );
		break;

	case X_BOX:
		if (IS_MIN_4K(rab)) {
			t1 = addr(&e1);
			v1 = (int) e1.e_addr;
			if (t1 == S_R8 && v1 == A) {
				outab(0xED);
				outab(op);
				break;
			}
			/* 6K: 0x6D 4ps x 2box == 8 codes */
		}
		aerr( );
		break;

	case X_RLB_RRB:
		if (IS_MIN_4K(rab)) {
			t1 = addr(&e1);
			v1 = (int) e1.e_addr;
			comma(1);
			t2 = addr(&e2);
			v2 = (int) e2.e_addr;
			if (t1 == S_R8 && v1 == A && 
				(t2 == S_R32_BCDE || t2 == S_R32_JKHL)) {
				outab((t2 == S_R32_BCDE) ? 0xDD : 0xFD);
				outab(op);
				break;
			}
		}
		aerr( );
		break;

	default:
		xerr('o', "Internal Opcode Error.");
		break;
	}
}

/*
 * general addressing evaluation
 * return(0) if general addressing mode output, else
 * return(esp->e_mode)
 */
int
genop(int pop, int op, struct expr *esp, int f)
{
	int t1;
	/*
	 * r
	 */
	if ((t1 = esp->e_mode) == S_R8) {
		if (pop)
			outab(pop);
		outab(op|esp->e_addr);
		return(0);
	}
	/*
	 * (hl)
	 */
	if (t1 == S_IDHL) {
		if ((esp->e_base.e_ap != NULL) || (esp->e_addr != 0))
			xerr('a', "(HL+D) is invalid.");
		if (pop)
			outab(pop);
		outab(op|0x06);
		return(0);
	}
	/*
	 * (ix) / (ix+d)
	 * (iy) / (iy+d)
	 */
	if (gixiy(t1) == S_IDHL) {
		if (pop) {
			outab(pop);
                        outrb(esp, 0);
			outab(op|0x06);
		} else {
			outab(op|0x06);
                        outrb(esp, 0);
		}
		return(0);
	}
	/*
	 *  n
	 * #n
	 */
	if ((t1 == S_IMMED) && (f)) {
		if (pop)
			outab(pop);
		outab(op|0x46);
		outrb(esp,0);
		return(0);
	}
	return(t1);
}

/*
 * IX and IY prebyte check
 */
int
gixiy(int v)
{
	if (v == IX) {
		v = HL;
		outab(0xDD);
	} else if (v == IY) {
		v = HL;
		outab(0xFD);
	} else if (v == S_IDIX) {
		v = S_IDHL;
		outab(0xDD);
	} else if (v == S_IDIY) {
		v = S_IDHL;
		outab(0xFD);
	}
	return(v);
}

#if 0
void 
clrPreByte(void)
{
	preByte.altd = 0;
	preByte.ioi  = 0;
	preByte.ioe  = 0;
}

void
chkIOPreByte(int addrMode)
{
	if (preByte.ioi || preByte.ioe) {
		if ((addrMode != S_IDHL) && (addrMode != S_IDIX) && (addrMode != S_IDIY))
			xerr('a', "IOE and IOI require (HL), (IX), or (IY) Indexing.");
	}
}

void
chkPreByte(struct mne *mp)
{
	if (preByte.altd) {
		outab(preByte.altd);
		if (!(mp->m_flag & P_ALTD))
			xerr('a', "This Instruction Does Not Support ALTD.");
	} else
	if (preByte.ioe) {
		outab(preByte.ioe);
		if (!(mp->m_flag & P_IO))
			xerr('a', "This Instruction Does Not Support IOE.");
	} else
	if (preByte.ioi) {
		outab(preByte.ioi);
		if (!(mp->m_flag & P_IO))
			xerr('a', "This Instruction Does Not Support IOI.");
	}
}
#endif

/*
 * Branch/Jump PCR Mode Check
 */
int
mchpcr(struct expr *esp)
{
	if (esp->e_base.e_ap == dot.s_area) {
		return(1);
	}
	if (esp->e_flag==0 && esp->e_base.e_ap==NULL) {
		/*
		 * Absolute Destination
		 *
		 * Use the global symbol '.__.ABS.'
		 * of value zero and force the assembler
		 * to use this absolute constant as the
		 * base value for the relocation.
		 */
		esp->e_flag = 1;
		esp->e_base.e_sp = &sym[1];
	}
	return(0);
}

/*
 * Machine dependent initialization
 */
void
minit(void)
{
	/*
	 * Byte Order
	 */
	hilo = 0;

	/*
         * Address Space
	 */
        exprmasks(3);
	if (pass == 0) {
		sym[2].s_addr = T_R2K;
	}
}


