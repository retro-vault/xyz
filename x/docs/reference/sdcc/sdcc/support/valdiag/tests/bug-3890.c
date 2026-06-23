/* bug-3890.c

   Initalization of extern variableat block scope was treated as assignment.
 */

#ifdef TEST1
void f(void)
{
extern int c = 2; /* ERROR */
}

int c;
#endif

