extern int test_qsort_newlib(void);

int main(void)
{
    int rc = 0;
    rc += test_qsort_newlib();
    return rc;
}
