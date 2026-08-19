#include <stdio.h>

int main(void)
{
    if (trygetchar() == 0)
        puts("IDLE");
    else
        puts(getchar() == 'x' ? "READY" : "FAIL");
    return 0;
}
