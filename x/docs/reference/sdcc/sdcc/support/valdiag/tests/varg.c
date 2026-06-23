#include <stdarg.h>

#ifdef TEST1
#pragma std_c23
void f(...);
#endif

#ifdef TEST2
#pragma std_c11
void f(...); /* WARNING */
#endif

