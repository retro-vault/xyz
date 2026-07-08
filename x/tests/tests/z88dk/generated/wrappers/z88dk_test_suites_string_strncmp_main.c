extern int test_strncmp(void);

int main(void)
{
    int rc = 0;
    rc += test_strncmp();
    return rc;
}
