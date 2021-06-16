#include <stdint.h>

long long _rrslonglong(long long l, char s)
{
	int32_t *top = (uint32_t *)((char *)(&l) + 4);
	uint16_t *middle = (uint16_t *)((char *)(&l) + 3);
	uint32_t *bottom = (uint32_t *)(&l);
	uint16_t *b = (uint16_t *)(&l);

	for(;s >= 16; s-= 16)
	{
		b[0] = b[1];
		b[1] = b[2];
		b[2] = b[3];
		b[3] = (b[3] & 0x8000) ? 0xffff : 0x000000;
	}

	(*bottom) >>= s;
	(*bottom) |= ((uint32_t)((*middle) >> s) << 16);
	(*top) |= (((*middle) & 0xffff0000) >> s);

	return(l);
}