#include <stdio.h>
#include <stdint.h>

#define REG(addr, reg)	__sfr __at(addr) reg

REG(0x00, GCSR);  // global control / status register
REG(0x01, RTCCR); // Real Time Clock Control Register
REG(0x02, RTC0R); // Real Time Clock Data Register 0
REG(0x03, RTC1R); // Real Time Clock Data Register 1
REG(0x04, RTC2R); // Real Time Clock Data Register 2
REG(0x05, RTC3R); // Real Time Clock Data Register 3
REG(0x06, RTC4R); // Real Time Clock Data Register 4
REG(0x07, RTC5R); // Real Time Clock Data Register 5
REG(0x08, WDTCR); // watch-dog timer control register
REG(0x09, WDTTR); // watch-dog timer test register
REG(0x0E, GOCR);  // Global Output Control Register
REG(0x0F, GCDR);  // global clock double register
REG(0x14, MB0CR); // Memory Bank 0 Control Register
REG(0x16, MB2CR); // Memory Bank 2 Control Register
REG(0x55, PCFR);  // Port C Function Register
REG(0xA0, TACSR); // Timer A Control/Status Register
REG(0xA9, TAT4R); // Timer A Time Constant 4 Register
REG(0xC0, SADR);  // Serial Port A Data Register
REG(0xC3, SASR);  // Serial Port A Status Register
REG(0xC4, SACR);  // Serial Port A Control Register

#if __SDCC_REVISION >= 13762
unsigned char __sdcc_external_startup(void)
#else
unsigned char _sdcc_external_startup(void)
#endif
{
	// Disable watchdog
	WDTTR = 0x51;
	WDTTR = 0x54;

	// normal oscillator, processor and peripheral from main clock, no periodic interrupt
	GCSR = 0x08;

	GCDR = 0x07; // Double clock to get more speed

	// Configure memory wait states
	MB0CR = 0x88; // Flash - 1 wait state (for 45 ns Flash @ 44.2 MHz) with write-protection
	MB2CR = 0x85; // RAM - 1 wait states (for 55 ns RAM @ 44.2 MHz)

	return(0);
}

unsigned long clock(void)
{
	unsigned long clock0, clock1;
	do
	{
		RTC0R = 0;
		clock0 = ((unsigned long)(RTC0R) << 0) | ((unsigned long)(RTC1R) << 8) | ((unsigned long)(RTC2R) << 16) | ((unsigned long)(RTC3R) << 24);
		clock1 = ((unsigned long)(RTC0R) << 0) | ((unsigned long)(RTC1R) << 8) | ((unsigned long)(RTC2R) << 16) | ((unsigned long)(RTC3R) << 24);
	} while (clock0 != clock1);
	return(clock1  / 32.768);
}

void init(void)
{
	PCFR = 0x40;    // Use pin PC6 as TXA
	TAT4R = 36 - 1; // Use divider for 38400 baud - value in register is one less than the divider used (e.g. a value of 0 will result in clock division by 1).
	TACSR = 0x01;   // Enable timer A
	SACR = 0x00;    // No interrupts, 8-bit async mode

	GOCR = 0x30;	// STATUS/DTR high signals to OpenRabbit that user program is running and will send data.
	// Give OpenRabbit and host some time (100 ms) to reconfigure baud rate
	{
		unsigned long c = clock();
		while (clock() - c < 100);
	}
}

#if defined(__SDCC) && __SDCC_REVISION < 9624 // Old SDCC weirdness
void putchar(char c)
{
  	while (SASR & 0x04);	// Wait for empty transmitter data register
	SADR = c;
}
#else // Standard C
int putchar(int c)
{
	// Convert newline to CRLF
	if (c == '\n') {
		putchar('\r');
	}

	while (SASR & 0x04);	// Wait for empty transmitter data register
	SADR = c;
	return c;
}
#endif

