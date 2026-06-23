/*-------------------------------------------------------------------------
   r4k.h - definitions of the built-in I/O ports for the Rabbit 4000
            for use with SDCC

   Copyright (C) 2025, Philipp Klaus Krause <philipp@colecovision.eu>

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING. If not, write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.

   As a special exception, if you link this library with other files,
   some of which are compiled with SDCC, to produce an executable,
   this library does not by itself cause the resulting executable to
   be covered by the GNU General Public License. This exception does
   not however invalidate any other reasons why the executable file
   might be covered by the GNU General Public License.
-------------------------------------------------------------------------*/

#ifndef __R4K_H__
#define __R4K_H__ 1

#include "r3ka.h"

_RABREG(0x001A, STACKSEGL);  // Stack Segment Low Register
_RABREG(0x001B, STACKSEGH);  // Stack Segment High Register
_RABREG(0x001B, MACR);       // Memory Alternate Control Register
_RABREG(0x001E, DATSEGL);    // Data Segment Low Register
_RABREG(0x001F, DATSEGH);    // Data Segment High Register

_RABREG(0x00F8, TCBAR);      // Timer C Block Access Register
_RABREG(0x00F9, TCBPR);      // Timer C Block Pointer Register

_RABREG(0x0100, DMCSR);      // DMA Master Control/Status Register
_RABREG(0x0101, DMALR);      // DMA Master Auto-Load Register
_RABREG(0x0102, DMHR);       // DMA Master Halt Register
_RABREG(0x0103, D0BCR);      // DMA 0 Complete Register
_RABREG(0x0104, DMCR);       // DMA Master Control Register
_RABREG(0x0105, DMTCR);      // DMA Master Timing Control Register
_RABREG(0x0106, DMR0CR);     // DMA Master Request 0 Control Register
_RABREG(0x0107, DMR1CR);     // DMA Master Request 1 Control Register
_RABREG(0x0108, D0TBR);      // DMA 0 Termination Byte Register
_RABREG(0x0109, D0TMR);      // DMA 0 Termination Mask Register
_RABREG(0x010A, D0BU0R);     // DMA 0 Buffer Unused 0 Register
_RABREG(0x010B, D0BU1R);     // DMA 0 Buffer Unused 1 Register
_RABREG(0x010C, D0IA0R);     // DMA 0 Initial Address 0 Register
_RABREG(0x010D, D0IA1R);     // DMA 0 Initial Address 1 Register
_RABREG(0x010E, D0IA2R);     // DMA 0 Initial Address 2 Register
_RABREG(0x0113, D1BCR);      // DMA 1 Complete Register
_RABREG(0x0115, DTRCR);      // DMA Timed Request Control Register
_RABREG(0x0116, DTRDLR);     // DMA Timed Request Divider Low Register
_RABREG(0x0117, DTRDHR);     // DMA Timed Request Divider High Register
_RABREG(0x0118, D1TBR);      // DMA 1 Termination Byte Register
_RABREG(0x0119, D1TMR);      // DMA 1 Termination Mask Register
_RABREG(0x011A, D1BU0R);     // DMA 1 Buffer Unused 0 Register
_RABREG(0x011B, D1BU1R);     // DMA 1 Buffer Unused 1 Register
_RABREG(0x011C, D1IA0R);     // DMA 1 Initial Address 0 Register
_RABREG(0x011D, D1IA1R);     // DMA 1 Initial Address 1 Register
_RABREG(0x011E, D1IA2R);     // DMA 1 Initial Address 2 Register
_RABREG(0x0123, D2BCR);      // DMA 2 Complete Register
_RABREG(0x0128, D2TBR);      // DMA 2 Termination Byte Register
_RABREG(0x0129, D2TMR);      // DMA 2 Termination Mask Register
_RABREG(0x012A, D2BU0R);     // DMA 2 Buffer Unused 0 Register
_RABREG(0x012B, D2BU1R);     // DMA 2 Buffer Unused 1 Register
_RABREG(0x012C, D2IA0R);     // DMA 2 Initial Address 0 Register
_RABREG(0x012D, D2IA1R);     // DMA 2 Initial Address 1 Register
_RABREG(0x012E, D2IA2R);     // DMA 2 Initial Address 2 Register
_RABREG(0x0133, D3BCR);      // DMA 3 Complete Register
_RABREG(0x0138, D3TBR);      // DMA 3 Termination Byte Register
_RABREG(0x0139, D3TMR);      // DMA 3 Termination Mask Register
_RABREG(0x013A, D3BU0R);     // DMA 3 Buffer Unused 0 Register
_RABREG(0x013B, D3BU1R);     // DMA 3 Buffer Unused 1 Register
_RABREG(0x013C, D3IA0R);     // DMA 3 Initial Address 0 Register
_RABREG(0x013D, D3IA1R);     // DMA 3 Initial Address 1 Register
_RABREG(0x013E, D3IA2R);     // DMA 3 Initial Address 2 Register
_RABREG(0x0143, D4BCR);      // DMA 4 Complete Register
_RABREG(0x0148, D4TBR);      // DMA 4 Termination Byte Register
_RABREG(0x0149, D4TMR);      // DMA 4 Termination Mask Register
_RABREG(0x014A, D4BU0R);     // DMA 4 Buffer Unused 0 Register
_RABREG(0x014B, D4BU1R);     // DMA 4 Buffer Unused 1 Register
_RABREG(0x014C, D4IA0R);     // DMA 4 Initial Address 0 Register
_RABREG(0x014D, D4IA1R);     // DMA 4 Initial Address 1 Register
_RABREG(0x014E, D4IA2R);     // DMA 4 Initial Address 2 Register
_RABREG(0x0153, D5BCR);      // DMA 5 Complete Register
_RABREG(0x0158, D5TBR);      // DMA 5 Termination Byte Register
_RABREG(0x0159, D5TMR);      // DMA 5 Termination Mask Register
_RABREG(0x015A, D5BU0R);     // DMA 5 Buffer Unused 0 Register
_RABREG(0x015B, D5BU1R);     // DMA 5 Buffer Unused 1 Register
_RABREG(0x015C, D5IA0R);     // DMA 5 Initial Address 0 Register
_RABREG(0x015D, D5IA1R);     // DMA 5 Initial Address 1 Register
_RABREG(0x015E, D5IA2R);     // DMA 5 Initial Address 2 Register
_RABREG(0x0163, D6BCR);      // DMA 6 Complete Register
_RABREG(0x0168, D6TBR);      // DMA 6 Termination Byte Register
_RABREG(0x0169, D6TMR);      // DMA 6 Termination Mask Register
_RABREG(0x016A, D6BU0R);     // DMA 6 Buffer Unused 0 Register
_RABREG(0x016B, D6BU1R);     // DMA 6 Buffer Unused 1 Register
_RABREG(0x016C, D6IA0R);     // DMA 6 Initial Address 0 Register
_RABREG(0x016D, D6IA1R);     // DMA 6 Initial Address 1 Register
_RABREG(0x016E, D6IA2R);     // DMA 6 Initial Address 2 Register
_RABREG(0x0173, D7BCR);      // DMA 7 Complete Register
_RABREG(0x0178, D7TBR);      // DMA 7 Termination Byte Register
_RABREG(0x0179, D7TMR);      // DMA 7 Termination Mask Register
_RABREG(0x017A, D7BU0R);     // DMA 7 Buffer Unused 0 Register
_RABREG(0x017B, D7BU1R);     // DMA 7 Buffer Unused 1 Register
_RABREG(0x017C, D7IA0R);     // DMA 7 Initial Address 0 Register
_RABREG(0x017D, D7IA1R);     // DMA 7 Initial Address 1 Register
_RABREG(0x017E, D7IA2R);     // DMA 7 Initial Address 2 Register
_RABREG(0x0180, D0SMR);      // DMA 0 State Machine Register
_RABREG(0x0181, D0CR);       // DMA 0 Control Register
_RABREG(0x0182, D0L0R);      // DMA 0 Length 0 Register
_RABREG(0x0183, D0L1R);      // DMA 0 Length 1 Register
_RABREG(0x0184, D0SA0R);     // DMA 0 Source Address 0 Register
_RABREG(0x0185, D0SA1R);     // DMA 0 Source Address 1 Register
_RABREG(0x0186, D0SA2R);     // DMA 0 Source Address 2 Register
_RABREG(0x0188, D0DA0R);     // DMA 0 Destination Address 0 Register
_RABREG(0x0189, D0DA1R);     // DMA 0 Destination Address 1 Register
_RABREG(0x018A, D0DA2R);     // DMA 0 Destination Address 2 Register
_RABREG(0x018C, D0LA0R);     // DMA 0 Link Address 0 Register
_RABREG(0x018D, D0LA1R);     // DMA 0 Link Address 1 Register
_RABREG(0x018E, D0LA2R);     // DMA 0 Link Address 2 Register
_RABREG(0x0190, D1SMR);      // DMA 1 State Machine Register
_RABREG(0x0191, D1CR);       // DMA 1 Control Register
_RABREG(0x0192, D1L0R);      // DMA 1 Length 0 Register
_RABREG(0x0193, D1L1R);      // DMA 1 Length 1 Register
_RABREG(0x0194, D1SA0R);     // DMA 1 Source Address 0 Register
_RABREG(0x0195, D1SA1R);     // DMA 1 Source Address 1 Register
_RABREG(0x0196, D1SA2R);     // DMA 1 Source Address 2 Register
_RABREG(0x0198, D1DA0R);     // DMA 1 Destination Address 0 Register
_RABREG(0x0199, D1DA1R);     // DMA 1 Destination Address 1 Register
_RABREG(0x019A, D1DA2R);     // DMA 1 Destination Address 2 Register
_RABREG(0x019C, D1LA0R);     // DMA 1 Link Address 0 Register
_RABREG(0x019D, D1LA1R);     // DMA 1 Link Address 1 Register
_RABREG(0x019E, D1LA2R);     // DMA 1 Link Address 2 Register
_RABREG(0x01A0, D2SMR);      // DMA 2 State Machine Register
_RABREG(0x01A1, D2CR);       // DMA 2 Control Register
_RABREG(0x01A2, D2L0R);      // DMA 2 Length 0 Register
_RABREG(0x01A3, D2L1R);      // DMA 2 Length 1 Register
_RABREG(0x01A4, D2SA0R);     // DMA 2 Source Address 0 Register
_RABREG(0x01A5, D2SA1R);     // DMA 2 Source Address 1 Register
_RABREG(0x01A6, D2SA2R);     // DMA 2 Source Address 2 Register
_RABREG(0x01A8, D2DA0R);     // DMA 2 Destination Address 0 Register
_RABREG(0x01A9, D2DA1R);     // DMA 2 Destination Address 1 Register
_RABREG(0x01AA, D2DA2R);     // DMA 2 Destination Address 2 Register
_RABREG(0x01AC, D2LA0R);     // DMA 2 Link Address 0 Register
_RABREG(0x01AD, D2LA1R);     // DMA 2 Link Address 1 Register
_RABREG(0x01AE, D2LA2R);     // DMA 2 Link Address 2 Register
_RABREG(0x01B0, D3SMR);      // DMA 3 State Machine Register
_RABREG(0x01B1, D3CR);       // DMA 3 Control Register
_RABREG(0x01B2, D3L0R);      // DMA 3 Length 0 Register
_RABREG(0x01B3, D3L1R);      // DMA 3 Length 1 Register
_RABREG(0x01B4, D3SA0R);     // DMA 3 Source Address 0 Register
_RABREG(0x01B5, D3SA1R);     // DMA 3 Source Address 1 Register
_RABREG(0x01B6, D3SA2R);     // DMA 3 Source Address 2 Register
_RABREG(0x01B8, D3DA0R);     // DMA 3 Destination Address 0 Register
_RABREG(0x01B9, D3DA1R);     // DMA 3 Destination Address 1 Register
_RABREG(0x01BA, D3DA2R);     // DMA 3 Destination Address 2 Register
_RABREG(0x01BC, D3LA0R);     // DMA 3 Link Address 0 Register
_RABREG(0x01BD, D3LA1R);     // DMA 3 Link Address 1 Register
_RABREG(0x01BE, D3LA2R);     // DMA 3 Link Address 2 Register
_RABREG(0x01C0, D4SMR);      // DMA 4 State Machine Register
_RABREG(0x01C1, D4CR);       // DMA 4 Control Register
_RABREG(0x01C2, D4L0R);      // DMA 4 Length 0 Register
_RABREG(0x01C3, D4L1R);      // DMA 4 Length 1 Register
_RABREG(0x01C4, D4SA0R);     // DMA 4 Source Address 0 Register
_RABREG(0x01C5, D4SA1R);     // DMA 4 Source Address 1 Register
_RABREG(0x01C6, D4SA2R);     // DMA 4 Source Address 2 Register
_RABREG(0x01C8, D4DA0R);     // DMA 4 Destination Address 0 Register
_RABREG(0x01C9, D4DA1R);     // DMA 4 Destination Address 1 Register
_RABREG(0x01CA, D4DA2R);     // DMA 4 Destination Address 2 Register
_RABREG(0x01CC, D4LA0R);     // DMA 4 Link Address 0 Register
_RABREG(0x01CD, D4LA1R);     // DMA 4 Link Address 1 Register
_RABREG(0x01CE, D4LA2R);     // DMA 4 Link Address 2 Register
_RABREG(0x01D0, D5SMR);      // DMA 5 State Machine Register
_RABREG(0x01D1, D5CR);       // DMA 5 Control Register
_RABREG(0x01D2, D5L0R);      // DMA 5 Length 0 Register
_RABREG(0x01D3, D5L1R);      // DMA 5 Length 1 Register
_RABREG(0x01D4, D5SA0R);     // DMA 5 Source Address 0 Register
_RABREG(0x01D5, D5SA1R);     // DMA 5 Source Address 1 Register
_RABREG(0x01D6, D5SA2R);     // DMA 5 Source Address 2 Register
_RABREG(0x01D8, D5DA0R);     // DMA 5 Destination Address 0 Register
_RABREG(0x01D9, D5DA1R);     // DMA 5 Destination Address 1 Register
_RABREG(0x01DA, D5DA2R);     // DMA 5 Destination Address 2 Register
_RABREG(0x01DC, D5LA0R);     // DMA 5 Link Address 0 Register
_RABREG(0x01DD, D5LA1R);     // DMA 5 Link Address 1 Register
_RABREG(0x01DE, D5LA2R);     // DMA 5 Link Address 2 Register
_RABREG(0x01E0, D6SMR);      // DMA 6 State Machine Register
_RABREG(0x01E1, D6CR);       // DMA 6 Control Register
_RABREG(0x01E2, D6L0R);      // DMA 6 Length 0 Register
_RABREG(0x01E3, D6L1R);      // DMA 6 Length 1 Register
_RABREG(0x01E4, D6SA0R);     // DMA 6 Source Address 0 Register
_RABREG(0x01E5, D6SA1R);     // DMA 6 Source Address 1 Register
_RABREG(0x01E6, D6SA2R);     // DMA 6 Source Address 2 Register
_RABREG(0x01E8, D6DA0R);     // DMA 6 Destination Address 0 Register
_RABREG(0x01E9, D6DA1R);     // DMA 6 Destination Address 1 Register
_RABREG(0x01EA, D6DA2R);     // DMA 6 Destination Address 2 Register
_RABREG(0x01EC, D6LA0R);     // DMA 6 Link Address 0 Register
_RABREG(0x01ED, D6LA1R);     // DMA 6 Link Address 1 Register
_RABREG(0x01EE, D6LA2R);     // DMA 6 Link Address 2 Register
_RABREG(0x01F0, D7SMR);      // DMA 7 State Machine Register
_RABREG(0x01F1, D7CR);       // DMA 7 Control Register
_RABREG(0x01F2, D7L0R);      // DMA 7 Length 0 Register
_RABREG(0x01F3, D7L1R);      // DMA 7 Length 1 Register
_RABREG(0x01F4, D7SA0R);     // DMA 7 Source Address 0 Register
_RABREG(0x01F5, D7SA1R);     // DMA 7 Source Address 1 Register
_RABREG(0x01F6, D7SA2R);     // DMA 7 Source Address 2 Register
_RABREG(0x01F8, D7DA0R);     // DMA 7 Destination Address 0 Register
_RABREG(0x01F9, D7DA1R);     // DMA 7 Destination Address 1 Register
_RABREG(0x01FA, D7DA2R);     // DMA 7 Destination Address 2 Register
_RABREG(0x01FC, D7LA0R);     // DMA 7 Link Address 0 Register
_RABREG(0x01FD, D7LA1R);     // DMA 7 Link Address 1 Register
_RABREG(0x01FE, D7LA2R);     // DMA 7 Link Address 2 Register

_RABREG(0x0200, NADR);       // Network Port A Data Register
_RABREG(0x0201, NALDR);      // Network Port A Last Data Register
_RABREG(0x0202, NATSR);      // Network Port A Transmit Status Register
_RABREG(0x0203, NARSR);      // Network Port A Receive Status Register
_RABREG(0x0204, NACSR);      // Network Port A Control/Status Register
_RABREG(0x0205, NASR);       // Network Port A Status Register
_RABREG(0x0206, NARR);       // Network Port A Reset Register
_RABREG(0x0207, NACR);       // Network Port A Control Register
_RABREG(0x0208, NAPCR);      // Network Port A Pin Control Register
_RABREG(0x020A, NATCR);      // Network Port A Transmit Control Register
_RABREG(0x020B, NARCR);      // Network Port A Receive Control Register
_RABREG(0x0210, NAPA0R);     // Network Port A Physical Address Register
_RABREG(0x0211, NAPA1R);     // Network Port A Physical Address Register
_RABREG(0x0212, NAPA2R);     // Network Port A Physical Address Register
_RABREG(0x0213, NAPA3R);     // Network Port A Physical Address Register
_RABREG(0x0214, NAPA4R);     // Network Port A Physical Address Register
_RABREG(0x0215, NAPA5R);     // Network Port A Physical Address Register
_RABREG(0x0218, NAMF0R);     // Network Port A Multicast Filter Register
_RABREG(0x0219, NAMF1R);     // Network Port A Multicast Filter Register
_RABREG(0x021A, NAMF2R);     // Network Port A Multicast Filter Register
_RABREG(0x021B, NAMF3R);     // Network Port A Multicast Filter Register
_RABREG(0x021C, NAMF4R);     // Network Port A Multicast Filter Register
_RABREG(0x021D, NAMF5R);     // Network Port A Multicast Filter Register
_RABREG(0x021E, NAMF6R);     // Network Port A Multicast Filter Register
_RABREG(0x021F, NAMF7R);     // Network Port A Multicast Filter Register
_RABREG(0x0220, NAMHR);      // Network Port A Multicast Hash Register
_RABREG(0x0221, NACDR);      // Network Port A Collision Detect Register
_RABREG(0x0222, NAAER);      // Network Port A Alignment Error Register
_RABREG(0x0223, NACER);      // Network Port A CRC Error Register
_RABREG(0x0224, NAC0R);      // Network Port A Checksum 0 Error Register
_RABREG(0x0225, NAC1R);      // Network Port A Checksum 1 Error Register
_RABREG(0x0226, NAMFR);      // Network Port A Missed Frame Register

_RABREG(0x0308, B0M0R);      // Breakpoint 0 Mask 0 Register
_RABREG(0x0309, B0M1R);      // Breakpoint 0 Mask 1 Register
_RABREG(0x030A, B0M2R);      // Breakpoint 0 Mask 2 Register
_RABREG(0x030B, B0CR);       // Breakpoint 0 Control Register
_RABREG(0x030C, B0A0R);      // Breakpoint 0 Address 0 Register
_RABREG(0x030D, B0A1R);      // Breakpoint 0 Address 1 Register
_RABREG(0x030E, B0A2R);      // Breakpoint 0 Address 2 Register
_RABREG(0x0318, B1M0R);      // Breakpoint 1 Mask 0 Register
_RABREG(0x0319, B1M1R);      // Breakpoint 1 Mask 1 Register
_RABREG(0x031A, B1M2R);      // Breakpoint 1 Mask 2 Register
_RABREG(0x031B, B1CR);       // Breakpoint 1 Control Register
_RABREG(0x031C, B1A0R);      // Breakpoint 1 Address 0 Register
_RABREG(0x031D, B1A1R);      // Breakpoint 1 Address 1 Register
_RABREG(0x031E, B1A2R);      // Breakpoint 1 Address 2 Register
_RABREG(0x0328, B2M0R);      // Breakpoint 2 Mask 0 Register
_RABREG(0x0329, B2M1R);      // Breakpoint 2 Mask 1 Register
_RABREG(0x032A, B2M2R);      // Breakpoint 2 Mask 2 Register
_RABREG(0x032B, B2CR);       // Breakpoint 2 Control Register
_RABREG(0x032C, B2A0R);      // Breakpoint 2 Address 0 Register
_RABREG(0x032D, B2A1R);      // Breakpoint 2 Address 1 Register
_RABREG(0x032E, B2A2R);      // Breakpoint 2 Address 2 Register
_RABREG(0x0338, B3M0R);      // Breakpoint 3 Mask 0 Register
_RABREG(0x0339, B3M1R);      // Breakpoint 3 Mask 1 Register
_RABREG(0x033A, B3M2R);      // Breakpoint 3 Mask 2 Register
_RABREG(0x033B, B3CR);       // Breakpoint 3 Control Register
_RABREG(0x033C, B3A0R);      // Breakpoint 3 Address 0 Register
_RABREG(0x033D, B3A1R);      // Breakpoint 3 Address 1 Register
_RABREG(0x033E, B3A2R);      // Breakpoint 3 Address 2 Register
_RABREG(0x0348, B4M0R);      // Breakpoint 4 Mask 0 Register
_RABREG(0x0349, B4M1R);      // Breakpoint 4 Mask 1 Register
_RABREG(0x034A, B4M2R);      // Breakpoint 4 Mask 2 Register
_RABREG(0x034B, B4CR);       // Breakpoint 4 Control Register
_RABREG(0x034C, B4A0R);      // Breakpoint 4 Address 0 Register
_RABREG(0x034D, B4A1R);      // Breakpoint 4 Address 1 Register
_RABREG(0x034E, B4A2R);      // Breakpoint 4 Address 2 Register
_RABREG(0x0358, B5M0R);      // Breakpoint 5 Mask 0 Register
_RABREG(0x0359, B5M1R);      // Breakpoint 5 Mask 1 Register
_RABREG(0x035A, B5M2R);      // Breakpoint 5 Mask 2 Register
_RABREG(0x035B, B5CR);       // Breakpoint 5 Control Register
_RABREG(0x035C, B5A0R);      // Breakpoint 5 Address 0 Register
_RABREG(0x035D, B5A1R);      // Breakpoint 5 Address 1 Register
_RABREG(0x035E, B5A2R);      // Breakpoint 5 Address 2 Register
_RABREG(0x0368, B6M0R);      // Breakpoint 6 Mask 0 Register
_RABREG(0x0369, B6M1R);      // Breakpoint 6 Mask 1 Register
_RABREG(0x036A, B6M2R);      // Breakpoint 6 Mask 2 Register
_RABREG(0x036B, B6CR);       // Breakpoint 6 Control Register
_RABREG(0x036C, B6A0R);      // Breakpoint 6 Address 0 Register
_RABREG(0x036D, B6A1R);      // Breakpoint 6 Address 1 Register
_RABREG(0x036E, B6A2R);      // Breakpoint 6 Address 2 Register

_RABREG(0x03F8, TCUER);      // Timer C User Enable Register

_RABREG(0x0500, TCCSR);      // Timer C Control/Status Register
_RABREG(0x0501, TCCR);       // Timer C Control Register
_RABREG(0x0502, TCDLR);      // Timer C Divider Low Register
_RABREG(0x0503, TCHLR);      // Timer C Divider High Register
_RABREG(0x0508, TCS0LR);     // Timer C Set 0 Low Register
_RABREG(0x0509, TCS0HR);     // Timer C Set 0 High Register
_RABREG(0x050A, TCR0LR);     // Timer C Reset 0 Low Register
_RABREG(0x050B, TCR0HR);     // Timer C Reset 0 High Register
_RABREG(0x050C, TCS1LR);     // Timer C Set 1 Low Register
_RABREG(0x050D, TCS1HR);     // Timer C Set 1 High Register
_RABREG(0x050E, TCR1LR);     // Timer C Reset 1 Low Register
_RABREG(0x050F, TCR1HR);     // Timer C Reset 1 High Register
_RABREG(0x0518, TCS2LR);     // Timer C Set 2 Low Register
_RABREG(0x0519, TCS2HR);     // Timer C Set 2 High Register
_RABREG(0x051A, TCR2LR);     // Timer C Reset 2 Low Register
_RABREG(0x051B, TCR2HR);     // Timer C Reset 2 High Register
_RABREG(0x051C, TCS3LR);     // Timer C Set 3 Low Register
_RABREG(0x051D, TCS3HR);     // Timer C Set 3 High Register
_RABREG(0x051E, TCR3LR);     // Timer C Reset 3 Low Register
_RABREG(0x051F, TCR3HR);     // Timer C Reset 3 High Register

#endif

