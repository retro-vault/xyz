/* arraybounds.c
   Some cases for null pointer check optimizations for [static] array parameters.
 */

#include <testfwk.h>

#include <stddef.h>

#pragma disable_warning 85
#pragma disable_warning 353

int f1(char c[static 1])
{
	return c == NULL;
}

int f2(char c[1])
{
	return c == NULL;
}

int f3(char c[static 2])
{
	return c + 1 == NULL;
}

int f4(long i, long j, char c[static 2]) // i and j ensure that c is not a register parameter.
{
	return c == NULL;
}

int g1(char c[static 1])
{
	return c == NULL;
}

int g2(char c[1])
{
	return c == NULL;
}

int g3(char c[static 2])
{
	return c + 1 == NULL;
}

int g4(long i, long j, char c[static 2]) // i and j ensure that c is not a register parameter.
{
	return c == NULL;
}

void testArraybounds (void)
{
	char c[2];

	ASSERT(!f1(c));
	ASSERT(!f2(c));
	ASSERT(f2(NULL));
	ASSERT(!f3(c));
	ASSERT(!f4(0, 0, c));

	ASSERT(!g1(c));
	ASSERT(!g2(c));
	ASSERT(g2(NULL));
	ASSERT(!g3(c));
	ASSERT(!g4(0, 0, c));
}

