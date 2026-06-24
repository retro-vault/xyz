#include <string.h>

typedef int (*fn4)(void *, const char *, const char *, const char *);

static int cb(void *user, const char *section, const char *name,
              const char *value) {
    int *state = (int *)user;

    if (*state != 12) return 10;
    if (strcmp(section, "main") != 0) return 20;
    if (strcmp(name, "title") != 0) return 21;
    if (strcmp(value, "X Tools") != 0) return 22;

    *state = 99;
    return 1;
}

static int dispatch(int left, int right, fn4 handler, void *user) {
    char line[200];
    char section[50];
    char previous[50];
    char guard[16];

    line[0] = 'X';
    section[0] = 'Y';
    previous[0] = 'Z';
    guard[0] = 'Q';

    if (left != 3) return 30;
    if (right != 4) return 31;
    if (line[0] != 'X') return 40;
    if (section[0] != 'Y') return 41;
    if (previous[0] != 'Z') return 42;
    if (guard[0] != 'Q') return 43;

    return handler(user, "main", "title", "X Tools");
}

int main(void) {
    int state = 12;
    int result = dispatch(3, 4, cb, &state);

    if (result != 1) return 1 + (result & 15);
    return state == 99 ? 0 : 2;
}
