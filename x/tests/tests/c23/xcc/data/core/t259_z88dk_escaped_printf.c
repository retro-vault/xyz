extern int printf(const char *format, ...);

static int (*saved_printf)(const char *, ...) = printf;

int call_saved_printf(int value)
{
    return saved_printf("%d", value);
}
