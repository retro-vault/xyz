#include <stdio.h>

int main(void)
{
    int minimum = -32767 - 1;
    int count = printf("X:%d\n", 32767);
    count += printf("A:%s:%d:%i:%%:%d\n", "ok", 0, minimum, 32767);
    return count == 30 ? 0 : 1;
}
