#include <stdint.h>

unsigned long long _rlulonglong(unsigned long long l, char s)
{
	uint32_t *const top = (uint32_t *)((char *)(&l) + 4);
	uint16_t *const middle = (uint16_t *)((char *)(&l) + 2);
	uint32_t *const bottom = (uint32_t *)(&l);
	uint16_t *const b = (uint16_t *)(&l);

	for(;s >= 16; s-= 16)
	{
		b[3] = b[2];
		b[2] = b[1];
		b[1] = b[0];
		b[0] = 0;
	}

	(*top) <<= s;
	(*top) |= (((uint32_t)((*middle) & 0xffffu) << s) >> 16);
	(*bottom) <<= s;

	return(l);
}