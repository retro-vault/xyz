/* bug-3890.c

   Initalization of extern variableat block scope was treated as assignment.
 */

#ifdef TEST1
void f(void, int b) /* ERROR */
{
}

void g2(void, void); /* ERROR */

void gv(void, ...); /* ERROR */

void g2(void, void) /* ERROR */
{
}

void gv(void, ...) /* ERROR */
{
}
#endif

