// Source by Philipp Krause under CC0 1.0

// A "Hello, World!"-Program. We use it to make the board
// output something that can not be mistaken for output
// From a benchmark.

// "Hello, world!" on serial port A @ 38400 baud.

#include <stdint.h>
#include <stdio.h>

#include "r3ka.h"

// _sdcc_external_startup, if present, will be called very early, before initalization
// of global objects. This makes it e.g. useful for dealing with watchdogs that might
// otherwise bite if there are many or large global objects that take a long time to initialize.
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

	return(0);
}

int putchar(int c)
{
	while (SASR & 0x04);	// Wait for empty transmitter data register
	SADR = c;
	return c;
}

unsigned long clock(void) // Get value of 32768 Hz real-time clock.
{
	unsigned long clock0, clock1;
	do
	{
		RTC0R = 0;
		clock0 = ((unsigned long)(RTC0R) << 0) | ((unsigned long)(RTC1R) << 8) | ((unsigned long)(RTC2R) << 16) | ((unsigned long)(RTC3R) << 24);
		clock1 = ((unsigned long)(RTC0R) << 0) | ((unsigned long)(RTC1R) << 8) | ((unsigned long)(RTC2R) << 16) | ((unsigned long)(RTC3R) << 24);
	} while (clock0 != clock1);
	return(clock1);
}

void main(void)
{
	GOCR = 0x30;	// STATUS/DTR high signals to OpenRabbit that user program is running and will send data.
	// Give OpenRabbit and host some time (100 ms) to reconfigure baud rate
	{
		unsigned long c = clock();
		while (clock() - c < 32 * 100);
	}

	PCFR = 0x40;	// Use pin PC6 as TXA

	TAT4R = 18 - 1;	// Value in register is one less than the divider used (e.g. a value of 0 will result in clock division by 1).
	TACSR = 0x01;	// Enable timer A

	SACR = 0x00;	// No interrupts, 8-bit async mode

	for (;;)
	{
		printf("Hello, World!\n");
		printf("\x04"); // EOT
	}
}

