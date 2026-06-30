extern int test_strlcpy(void);

int main(void)
{
    int rc = 0;
    rc += test_strlcpy();
    return rc;
}
