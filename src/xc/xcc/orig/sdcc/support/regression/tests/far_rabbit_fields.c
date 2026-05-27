// far_rabbit_fields.c - test bit-fields in __far space that contains generic space.

#include <testfwk.h>

#include <stdbool.h>

#if !defined(__SDCC_r2k) && !defined(__SDCC_r2ka) && !defined(__SDCC_r3ka) && !defined(__SDCC_r4k) && !defined(__SDCC_r5k) && !defined(__SDCC_r6k) && !defined(__SDCC_tlcs90) /*&& !defined(__SDCC_ez80) TODO bug #3882*/
#define __far
char farmemblock[2];
#else
#undef __far
// Ensure that farmemblock and farmemblock+sizeof(farmemblock) differ in more than just the lowest 16 bits. Also gives further objects in __far an address > 2^16.
char memblock[6000];
__far char farmemblock[40000];
#endif

struct sb
{
	unsigned int ui12 : 12;
	unsigned int ui4 : 4;
	signed int si13 : 13;
	signed int si3 : 3;
	bool b : 1;
};

__far struct sb sb = {0xa5, 0x5, -23, -1, false};
__far const struct sb csb = {0xa5, 0x5, -23, 1, true};

void testRead(void)
{
	ASSERT(sb.ui12 == 0xa5);
	ASSERT(sb.ui4 == 0x5);
	ASSERT(sb.si13 == -23);
	ASSERT(sb.si3 == -1);
	ASSERT(sb.b == false);

	ASSERT(csb.ui12 == 0xa5);
	ASSERT(csb.ui4 == 0x5);
	ASSERT(csb.si13 == -23);
	ASSERT(csb.si3 == 1);
	ASSERT(csb.b == true);
}

volatile int vi = 0x5a;

void testWrite(void)
{
	sb.ui12 = vi;
	sb.si13 = vi;
	sb.b = true;
	ASSERT (sb.ui12 == 0x5a);
	ASSERT (sb.si13 == 0x5a);
	ASSERT (sb.b == true);
	vi = 24;
	sb.ui12 = 24;
	sb.si13 = -24;
	sb.b = false;
	ASSERT (sb.ui12 == vi);
	ASSERT (sb.si13 == -vi);
	ASSERT (sb.b == false);
}

