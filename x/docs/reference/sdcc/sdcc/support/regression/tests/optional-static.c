/** optional-static.c: _Optional cancels out some guarantees on [static assignment-expression].
 */

#include <testfwk.h>

#include <stddef.h>
#include <stdbool.h>

bool flag;

#ifdef __SDCC
void f(_Optional int a[static 1])
{
	flag = (bool)a;
}
#endif

void testOptionalStatic(void)
{
#ifdef __SDCC
	f(NULL);
#endif
	ASSERT(!flag);
}

