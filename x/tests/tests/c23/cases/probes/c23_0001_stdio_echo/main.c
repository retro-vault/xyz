#include <stdio.h>

int main(void) {
    const int first = getchar();
    const int second = getchar();

    if (first == EOF || second == EOF) {
        return 2;
    }

    putchar(first);
    putchar(second);
    putchar('!');
    putchar('\n');
    return 0;
}
