// Test diagnostics assigning string literals to pointers to non-const.

#ifdef TEST1
char *p = "test"; /* WARNING */

void f(void)
{
	p = "test2"; /* WARNING */
}
#endif

#ifdef TEST2
#include <uchar.h>

char16_t *p = u"test"; /* WARNING */

void f(void)
{
	p = u"test2"; /* WARNING */
}
#endif

