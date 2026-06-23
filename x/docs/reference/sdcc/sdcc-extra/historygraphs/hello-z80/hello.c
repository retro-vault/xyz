// Source by Philipp Krause under CC0 1.0

// A "Hello, World!"-Program. We use it to make the board
// output something that can not be mistaken for output
// From a benchmark.

// "Hello, world!" @ 115200 baud.

#include <stdint.h>
#include <stdio.h>

__sfr __at 0x00 Exec_port;
__sfr __at 0x01 StorOpc_port;

#if defined(__SDCC) && __SDCC_REVISION < 9624 // Old SDCC weirdness
void putchar (char c)
{
__asm
	di
__endasm;
	StorOpc_port = 0x1;
	Exec_port = c;
__asm
	ei
__endasm;
}
#else // Standard C
int putchar (int c)
{
__asm
	di
__endasm;
	StorOpc_port = 0x1;
	Exec_port = c;
__asm
	ei
__endasm;
	return(c);
}
#endif

void main(void)
{
	for (;;) {
		printf("Hello, World!\n");
	}
}

