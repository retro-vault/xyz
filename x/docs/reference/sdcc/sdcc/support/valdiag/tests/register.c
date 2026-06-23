// Invalid uses of register storage class

// register combined with static
#ifdef TEST1
void f(void)
{
	register static int i; /* ERROR */
}
#endif

// register combined with extern
#ifdef TEST2
void f(void)
{
	extern register int i; /* ERROR */
}
#endif

// register on external declaration
#ifdef TEST3
register int i; /* WARNING */
#endif

