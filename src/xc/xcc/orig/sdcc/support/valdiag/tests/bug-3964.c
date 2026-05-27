/* bug-3964.c

   Qualifiers on return types were not ignored, resulting in invalid errors about mismatching declarations.
 */

#ifdef TEST1
volatile int f(void);             /* WARNING */
volatile void g(void);            /* WARNING */

const int f1(int);                /* WARNING */
volatile int f1(int);             /* WARNING */

const int f2(void);               /* WARNING */
volatile int f2(void) {return 0;} /* WARNING */
#endif

#ifdef TEST2
const int *f1(int);               /* IGNORE */
volatile int *f1(int);            /* ERROR */
#endif

