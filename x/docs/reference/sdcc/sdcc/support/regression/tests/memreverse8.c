/* memreverse8.c
   memory reversal functions from stdbit.h
 */

#include <testfwk.h>

#pragma std_c2y

#if __STDC_VERSION__ > 202311L || defined(__SDCC)
#include <stdbit.h>
#endif

void testMemreverse8(void)
{
#if __STDC_VERSION__ > 202311L || defined(__SDCC)
	unsigned char a[3] = {0xaa, 0x55, 0x00};
	stdc_memreverse8(sizeof(a), a);
	ASSERT(a[0] == 0x00);
	ASSERT(a[1] == 0x55);
	ASSERT(a[2] == 0xaa);

	uint8_t u8 = 0xa5;
	ASSERT(stdc_memreverse8u8(u8) == 0xa5);
	uint16_t u16 = 0xa55a;
	ASSERT(stdc_memreverse8u16(u16) == 0x5aa5);
	uint32_t u32 = 0x55a55aaa;
	ASSERT(stdc_memreverse8u32(u32) == 0xaa5aa555);
#endif
}

