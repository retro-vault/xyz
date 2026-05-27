/*-------------------------------------------------------------------------
   r5k.h - definitions of the built-in I/O ports for the Rabbit 5000
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

#ifndef __R5K_H__
#define __R5K_H__ 1

#include "r4k.h"

_RABREG(0x0200, NBDR);     // Network Port B Data Register
_RABREG(0x0201, NBLDR);    // Network Port B Last Data Register
_RABREG(0x0202, NBTSR);    // Network Port B Transmit Status Register
_RABREG(0x0204, NBCSR);    // Network Port B Control/Status Register
_RABREG(0x0206, NBCR);     // Network Port B Command Register
_RABREG(0x0208, NBTPLR);   // Network Port B Transmit Pause LSB Register
_RABREG(0x0209, NBTPMR);   // Network Port B Transmit Pause MSB Register
_RABREG(0x020A, NBTCR);    // Network Port B Transmit Control Register
_RABREG(0x020B, NBRCR);    // Network Port B Receive Control Register
_RABREG(0x020C, NBTESR);   // Network Port B Transmit Extra Status Register
_RABREG(0x0210, NBPA0R);   // Network Port B Physical Address Register
_RABREG(0x0211, NBPA1R);   // Network Port B Physical Address Register
_RABREG(0x0212, NBPA2R);   // Network Port B Physical Address Register
_RABREG(0x0213, NBPA3R);   // Network Port B Physical Address Register
_RABREG(0x0214, NBPA4R);   // Network Port B Physical Address Register
_RABREG(0x0215, NBPA5R);   // Network Port B Physical Address Register
_RABREG(0x0218, NBMF0R);   // Network Port B Multicast Filter Register
_RABREG(0x0219, NBMF1R);   // Network Port B Multicast Filter Register
_RABREG(0x021A, NBMF2R);   // Network Port B Multicast Filter Register
_RABREG(0x021B, NBMF3R);   // Network Port B Multicast Filter Register
_RABREG(0x021C, NBMF4R);   // Network Port B Multicast Filter Register
_RABREG(0x021D, NBMF5R);   // Network Port B Multicast Filter Register
_RABREG(0x021E, NBMF6R);   // Network Port B Multicast Filter Register
_RABREG(0x021F, NBMF7R);   // Network Port B Multicast Filter Register
_RABREG(0x0228, NBDRR);    // Network Port B Direct Rx Register
_RABREG(0x0229, NBDTR);    // Network Port B Direct Tx Register
_RABREG(0x022A, NBDMR);    // Network Port B Direct MRII Register
_RABREG(0x0240, NBCF0R);   // Network Port B Configuration 0 Register
_RABREG(0x0241, NBCF1R);   // Network Port B Configuration 1 Register
_RABREG(0x0242, NBCF2R);   // Network Port B Configuration 2 Register
_RABREG(0x0243, NBCF3R);   // Network Port B Configuration 3 Register
_RABREG(0x0244, NBG0R);    // Network Port B Gap 0 Register
_RABREG(0x0246, NBG2R);    // Network Port B Gap 2 Register
_RABREG(0x0247, NBG1R);    // Network Port B Gap 1 Register
_RABREG(0x0248, NBRMR);    // Network Port B Retransmit Max Register
_RABREG(0x0249, NBCWR);    // Network Port B Collision Window Register
_RABREG(0x024A, NBFLLR);   // Network Port B Frame Limit LSB Register
_RABREG(0x024B, NBFLMR);   // Network Port B Frame Limit MSB Register
_RABREG(0x0250, NBMCFR);   // Network Port B MII Configuration Register
_RABREG(0x0251, NBMRR);    // Network Port B MII Reset Register
_RABREG(0x0252, NBMCR);    // Network Port B MII Command Register
_RABREG(0x0254, NBMRAR);   // Network Port B MII Register Address Register
_RABREG(0x0254, NBMPAR);   // Network Port B MII PHY Address Register
_RABREG(0x0256, NBMWLR);   // Network Port B MII Write LSB Register
_RABREG(0x0257, NBMWMR);   // Network Port B MII Write MSB Register
_RABREG(0x0258, NBMRLR);   // Network Port B MII Read LSB Register
_RABREG(0x0259, NBMRMR);   // Network Port B MII Read MSB Register
_RABREG(0x025A, NBMSR);    // Network Port B MII Status Register
_RABREG(0x0260, NBSA0R);   // Network Port B Station Address 0 Register
_RABREG(0x0261, NBSA1R);   // Network Port B Station Address 1 Register
_RABREG(0x0262, NBSA2R);   // Network Port B Station Address 2 Register
_RABREG(0x0263, NBSA3R);   // Network Port B Station Address 3 Register
_RABREG(0x0264, NBSA4R);   // Network Port B Station Address 4 Register
_RABREG(0x0265, NBSA5R);   // Network Port B Station Address 5 Register

_RABREG(0x0430, ENPR);     // Enable Network Port Register

_RABREG(0x0800, A0ILR);    // Analog Component 0 I LSB Register
_RABREG(0x0801, A0IMR);    // Analog Component 0 I MSB Register
_RABREG(0x0802, A0QLR);    // Analog Component 0 Q LSB Register
_RABREG(0x0803, A0QMR);    // Analog Component 0 Q MSB Register
_RABREG(0x0804, A0CR);     // Analog Component 0 Control Register
_RABREG(0x0810, A1ILR);    // Analog Component 1 I LSB Register
_RABREG(0x0811, A1IMR);    // Analog Component 1 I MSB Register
_RABREG(0x0812, A1QLR);    // Analog Component 1 Q LSB Register
_RABREG(0x0813, A1QMR);    // Analog Component 1 Q MSB Register
_RABREG(0x0814, A1CR);     // Analog Component 1 Control Register
_RABREG(0x0820, A2LR);     // Analog Component 2 LSB Register
_RABREG(0x0821, A2MR);     // Analog Component 2 MSB Register
_RABREG(0x0824, A2CR);     // Analog Component 2 Control Register

_RABREG(0x0A00, NCV0R);    // Network Port C Version 0 Register
_RABREG(0x0A01, NCV1R);    // Network Port C Version 1 Register
_RABREG(0x0A04, NCGC0R);   // Network Port C General Control 0 Register
_RABREG(0x0A05, NCGC1R);   // Network Port C General Control 1 Register
_RABREG(0x0A06, NCGC2R);   // Network Port C General Control 2 Register
_RABREG(0x0A07, NCGC3R);   // Network Port C General Control 3 Register
_RABREG(0x0A08, NCGS0R);   // Network Port C General Status 0 Register
_RABREG(0x0A09, NCGS1R);   // Network Port C General Status 1 Register
_RABREG(0x0A0A, NCGS2R);   // Network Port C General Status 2 Register
_RABREG(0x0A0B, NCGS3R);   // Network Port C General Status 3 Register
_RABREG(0x0A0C, NCRSSI0R); // Network Port C RRSI 0 Register
_RABREG(0x0A0D, NCRSSI1R); // Network Port C RRSI 1 Register
_RABREG(0x0A0E, NCRSSI2R); // Network Port C RRSI 2 Register
_RABREG(0x0A0F, NCRSSI3R); // Network Port C RRSI 3 Register
_RABREG(0x0A10, NCIMR);    // Network Port C Interrupt Mask Register
_RABREG(0x0A11, NCISR);    // Network Port C Interrupt Status Register
_RABREG(0x0A18, NCSPID0R); // Network Port C SPI Data 0 Register
_RABREG(0x0A19, NCSPID1R); // Network Port C SPI Data 1 Register
_RABREG(0x0A1A, NCSPID2R); // Network Port C SPI Data 2 Register
_RABREG(0x0A1B, NCSPID3R); // Network Port C SPI Data 3 Register
_RABREG(0x0A1B, NCSPICR);  // Network Port C SPI Control Register
_RABREG(0x0A20, NCDFR);    // Network Port C Data FIFO Register
_RABREG(0x0A28, NCC1R0);   // Network Port C Configuration-1 Register 0
_RABREG(0x0A29, NCC1R1);   // Network Port C Configuration-1 Register 1
_RABREG(0x0A2A, NCC1R2);   // Network Port C Configuration-1 Register 2
_RABREG(0x0A2B, NCC1R3);   // Network Port C Configuration-1 Register 3
_RABREG(0x0A2C, NCC2R0);   // Network Port C Configuration-2 Register 0
_RABREG(0x0A2D, NCC2R1);   // Network Port C Configuration-2 Register 1
_RABREG(0x0A2E, NCC2R2);   // Network Port C Configuration-2 Register 2
_RABREG(0x0A2F, NCC2R3);   // Network Port C Configuration-2 Register 3
_RABREG(0x0A30, NCAFR);    // Network Port C AES FIFO Register
_RABREG(0x0A38, NCAMR);    // Network Port C AES Mode Register
_RABREG(0x0A3C, NCOCR0);   // Network Port C Output Control Register 0
_RABREG(0x0A3D, NCOCR1);   // Network Port C Output Control Register 1
_RABREG(0x0A3E, NCOCR2);   // Network Port C Output Control Register 2
_RABREG(0x0A3F, NCOCR3);   // Network Port C Output Control Register 3
// TODO: There are more, but Rabbit 5000/6000 WiFi documentation in incomplete anway, so they might not be that useful.

#endif

