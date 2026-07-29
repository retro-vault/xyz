static int seen;

int printf(const char *text)
{
    seen += *text;
    return seen;
}

int main(void)
{
    return printf("x");
}
