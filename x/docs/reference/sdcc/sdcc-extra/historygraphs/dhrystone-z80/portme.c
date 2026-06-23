#include <stdio.h>
#include <stdint.h>

__sfr __at 0x00 Exec_port;
__sfr __at 0x01 StorOpc_port;

volatile unsigned long ticks;

void init (void)
{
	// Setup tick interrupt.
	StorOpc_port = 0x0f; // Set tick opcode
	Exec_port = 10; // Once every 10 ms.
	
	StorOpc_port = 0x0e; // Set interrupt opcode
	Exec_port = 0x02; // Tick interrupt only.
	
__asm
	im	1
	ei
__endasm;
}

void tick (void)
{
	ticks++;
}

unsigned long clock(void)
{
	long ret;
	do
		ret = ticks;
	while (ret != ticks);
	return(ret);
}

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

