/*
   bug-3920.c
   A false positice warning about incomplete arrays having implicit size 1 due to implicit initializer on etxern declaration.
 */

#ifdef TEST1
extern int i[];

int i[]; /* WARNING */
#endif

#ifdef TEST2
extern int i[];
#endif

#ifdef TEST3
int i[]; /* WARNING */
#endif

