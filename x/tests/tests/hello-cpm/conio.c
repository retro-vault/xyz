#include <conio.h>
#include <stdio.h>

int main(void)
{
    if (kbhit() == 0)
        puts("IDLE");
    else
        puts(getchar() == 'x' ? "READY" : "FAIL");
    return 0;
}
