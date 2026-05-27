/* bug-3801.c

   A bug resulting in negative array sizes not being diagnosed.
 */

#ifdef TEST1
char c[-2]; /* ERROR */
#endif

#ifdef TEST2
void f(void)
{
    char c[-2]; /* ERROR */
}
#endif

