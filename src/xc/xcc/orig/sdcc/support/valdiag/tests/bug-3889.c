/* bug-3889.c

   Incomplete check of block-scope extern declaration vs. file-scope declaration.
 */

#ifdef TEST1
char c; /* IGNORE */
void f(void)
{
extern int c; /* ERROR */
}
#endif

#ifdef TEST2
void f(void) /* IGNORE */
{
extern int c; /* IGNORE */
}
char c; /* ERROR */
#endif

#ifdef TEST3
void g(void) /* IGNORE */
{
  extern int x; /* IGNORE */
}

void h(void)
{
  extern char x; /* ERROR */
}
#endif

