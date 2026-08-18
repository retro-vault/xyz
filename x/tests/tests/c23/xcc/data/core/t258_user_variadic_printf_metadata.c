static int seen;

int printf(const char *text, ...)
{
    seen += *text;
    return seen;
}

int call_user_printf(void)
{
    return printf("x", 1);
}
