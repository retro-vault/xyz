extern int test_strlcat(void);

int main(void)
{
    int rc = 0;
    rc += test_strlcat();
    return rc;
}
