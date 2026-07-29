#include <stdio.h>

int print_small(const char *text, int value)
{
    return printf("%s:%d:%i:%%\n", text, value, value);
}

int print_width(int value)
{
    return printf("%04d\n", value);
}

int print_dynamic(const char *format, int value)
{
    return printf(format, value);
}
