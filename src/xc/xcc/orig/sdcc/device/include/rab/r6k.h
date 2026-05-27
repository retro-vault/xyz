/*-------------------------------------------------------------------------
   r6k.h - definitions of the built-in I/O ports for the Rabbit 6000
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

#ifndef __R6K_H__
#define __R6K_H__ 1

#include "r5k.h"

// TODO: DMA channels 8-15.

_RABREG(0x0433, NDWR);     // Network Port D Wait Register

_RABREG(0x0434, MSCR);     // Master System Configuration Register
_RABREG(0x0435, MSSR);     // Master System Status Register
_RABREG(0x0436, MSRR);     // Master System Reset Register

_RABREG(0x0438, POCR);     // Port Override Control Register

_RABREG(0x0540, ADCLR);    // ADC LSB Register
_RABREG(0x0541, ADCMR);    // ADC MSB Register
_RABREG(0x0542, ADCCSR);   // ADC Command/Status Register
_RABREG(0x0543, ADCCR);    // ADC Control Register
_RABREG(0x0550, ADC0LR);   // ADC 0 LSB Register
_RABREG(0x0551, ADC0MR);   // ADC 0 MSB Register
_RABREG(0x0552, ADC1LR);   // ADC 1 LSB Register
_RABREG(0x0553, ADC1MR);   // ADC 1 MSB Register
_RABREG(0x0554, ADC2LR);   // ADC 2 LSB Register
_RABREG(0x0555, ADC2MR);   // ADC 2 MSB Register
_RABREG(0x0556, ADC3LR);   // ADC 3 LSB Register
_RABREG(0x0557, ADC3MR);   // ADC 3 MSB Register
_RABREG(0x0558, ADC4LR);   // ADC 4 LSB Register
_RABREG(0x0559, ADC4MR);   // ADC 4 MSB Register
_RABREG(0x055A, ADC5LR);   // ADC 5 LSB Register
_RABREG(0x055B, ADC5MR);   // ADC 5 MSB Register
_RABREG(0x055C, ADC6LR);   // ADC 6 LSB Register
_RABREG(0x055D, ADC6MR);   // ADC 6 MSB Register
_RABREG(0x055E, ADC7LR);   // ADC 7 LSB Register
_RABREG(0x055F, ADC7MR);   // ADC 7 MSB Register

_RABREG(0x0580, SGC0R);    // Serial Port G Control 0 Register
_RABREG(0x0581, SGC1R);    // Serial Port G Control 1 Register
_RABREG(0x0582, SGC2R);    // Serial Port G Control 2 Register
_RABREG(0x0583, SGC3R);    // Serial Port G Control 3 Register
_RABREG(0x0584, SGS0R);    // Serial Port G Status 0 Register
_RABREG(0x0585, SGS1R);    // Serial Port G Status 1 Register
_RABREG(0x0586, SGS2R);    // Serial Port G Status 2 Register
_RABREG(0x0587, SGS3R);    // Serial Port G Status 3 Register
_RABREG(0x0588, SGCD0R);   // Serial Port G Clock Divider 0 Register
_RABREG(0x0589, SGCD1R);   // Serial Port G Clock Divider 1 Register
_RABREG(0x058A, SGCD2R);   // Serial Port G Clock Divider 2 Register
_RABREG(0x058B, SGCD3R);   // Serial Port G Clock Divider 3 Register
_RABREG(0x058C, SGDR);     // Serial Port G Data Register
_RABREG(0x0590, SGSA0R);   // Serial Port G Slave Address 0 Register
_RABREG(0x0591, SGSA1R);   // Serial Port G Slave Address 1 Register
_RABREG(0x0592, SGSA2R);   // Serial Port G Slave Address 2 Register
_RABREG(0x0593, SGSA3R);   // Serial Port G Slave Address 3 Register
_RABREG(0x0594, SGTC0R);   // Serial Port G Timing Control 0 Register
_RABREG(0x0595, SGTC1R);   // Serial Port G Timing Control 1 Register
_RABREG(0x0596, SGTC2R);   // Serial Port G Timing Control 2 Register
_RABREG(0x0597, SGTC3R);   // Serial Port G Timing Control 3 Register
_RABREG(0x0598, SGSBM0R);  // Serial Port G Bus Monitor 0 Register
_RABREG(0x0599, SGSBM1R);  // Serial Port G Bus Monitor 1 Register
_RABREG(0x059A, SGSBM2R);  // Serial Port G Bus Monitor 2 Register
_RABREG(0x059B, SGSBM3R);  // Serial Port G Bus Monitor 3 Register
_RABREG(0x059F, SGMCR);    // Serial Port G Mein Control Register

_RABREG(0x05C0, ECD0R);    // ECC Data 0 Register
_RABREG(0x05C1, ECD1R);    // ECC Data 1 Register
_RABREG(0x05C2, ECD2R);    // ECC Data 2 Register
_RABREG(0x05C3, ECD3R);    // ECC Data 3 Register
_RABREG(0x05C4, ECCR);     // ECC Control Register
_RABREG(0x05C5, ECPR);     // ECC CP Read Register
_RABREG(0x05C6, ECPSR);    // ECC CP Read Shifted Register
_RABREG(0x05C7, ECW0R);    // ECC Write 0 Register
_RABREG(0x05C8, ECW1R);    // ECC Write 1 Register
_RABREG(0x05C9, ECW2R);    // ECC Write 2 Register
_RABREG(0x05CA, ECW3R);    // ECC Write 3 Register
_RABREG(0x05CB, ECC0R);    // ECC Count 0 Register
_RABREG(0x05CC, ECC1R);    // ECC Count 1 Register

_RABREG(0x1060, USBWR);    // USB Wrapper Register

// TODO: FIM Registers, but Rabbit 6000 FIM documentation in incomplete anway, so they might not be that useful.

#endif

