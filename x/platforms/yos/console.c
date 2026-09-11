#include <stddef.h>
#include <stdio.h>
#include <yos.h>

static yos_putchar_hook_t output_hook;

yos_putchar_hook_t yos_set_putchar_hook(yos_putchar_hook_t hook)
{
    yos_t *yos = yos_get_api();
    yos_putchar_hook_t previous;

    if (yos)
        yos->enter_critical_section();
    previous = output_hook;
    output_hook = hook;
    if (yos)
        yos->leave_critical_section();
    return previous;
}

int putchar(int value)
{
    yos_t *yos = yos_get_api();
    yos_putchar_hook_t hook;

    if (yos)
        yos->enter_critical_section();
    hook = output_hook;
    if (yos)
        yos->leave_critical_section();
    if (hook)
        hook((char)value);
    return (unsigned char)value;
}

int getchar(void)
{
    return EOF;
}

int trygetchar(void)
{
    return 0;
}
