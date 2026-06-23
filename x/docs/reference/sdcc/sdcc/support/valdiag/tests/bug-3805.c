/* bug-3805.c

   An issue in processing function parameters of a returned function type resulted in an invalid diagnostic for --stack-auto. (and ports where this is the default)
 */

int f(int i, int j)
{
    return i + j;
}

typeof (int (*)(int, int)) g1(void)
{
    return &f; // Invalid diagnostic happened here.
}

int (*g2(void))(int, int)
{
	return &f; // Invalid diagnostic happened here.
}

