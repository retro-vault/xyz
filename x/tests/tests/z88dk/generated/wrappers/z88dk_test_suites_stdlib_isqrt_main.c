extern int test_isqrt(void);
extern int test_isqrt2(void);

int main(void)
{
    int rc = 0;
    rc += test_isqrt();
    rc += test_isqrt2();
    return rc;
}
