// far_rabbit_pointers.c - test pointers to __far space that contains generic space.

#include <testfwk.h>

#include <stdbool.h>

#if !defined(__SDCC_r2k) && !defined(__SDCC_r2ka) && !defined(__SDCC_r3ka) && !defined(__SDCC_r4k) && !defined(__SDCC_r5k) && !defined(__SDCC_r6k) && !defined(__SDCC_tlcs90) /*&& !defined(__SDCC_ez80) TODO bug #3882*/
#define __far
#else
#undef __far
#endif

int i;
const int ci = 7;
char array[4];
const char carray[4];

__far void *to_far(void *p);
void *from_far(__far void *p);
__far const char *objaddr_to_far(void);
__far const char* cobjaddr_to_far(void);
__far const char *aaddr_to_far(void);
__far const char* caaddr_to_far(void);

void testPtr(void)
{
	// Run-time vs. compile-time casts
	ASSERT(to_far(&i) == objaddr_to_far());
	ASSERT(to_far(&ci) == cobjaddr_to_far());
	ASSERT(to_far(array) == aaddr_to_far());
	ASSERT(to_far(carray) == caaddr_to_far());

	// Round-trip casts
	ASSERT(from_far(to_far(&i)) == &i);
	ASSERT(from_far(to_far(&ci)) == &ci);
	ASSERT(from_far(to_far(array)) == array);
	ASSERT(from_far(to_far(carray)) == carray);

	// Commutativity of pointer arithmetic with casts.
	ASSERT(from_far((__far char *)(to_far(array)) + 3) == array + 3);
	ASSERT(from_far((__far char *)(to_far(carray)) + 3) == carray + 3);
}

__far void *to_far(void *p)
{
	return p;
}

void *from_far(__far void *p)
{
	return p;
}

__far const char *objaddr_to_far(void)
{
	return(const __far void *)(&i);
}

__far const char* cobjaddr_to_far(void)
{
	return(const __far void *)(&ci);
}

__far const char *aaddr_to_far(void)
{
	return(const __far void *)(array);
}

__far const char* caaddr_to_far(void)
{
	return(const __far void *)(carray);
}

