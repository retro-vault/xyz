#include <string.h>

#include "ini.h"

struct seen {
    int title;
    int answer;
    int path;
};

static int handler(void *user, const char *section,
                   const char *name, const char *value) {
    struct seen *s = (struct seen *)user;
    if (strcmp(section, "main") == 0 &&
        strcmp(name, "title") == 0 &&
        strcmp(value, "X Tools") == 0) {
        s->title = 1;
    } else if (strcmp(section, "main") == 0 &&
               strcmp(name, "answer") == 0 &&
               strcmp(value, "42") == 0) {
        s->answer = 1;
    } else if (strcmp(section, "paths") == 0 &&
               strcmp(name, "root") == 0 &&
               strcmp(value, "/z80") == 0) {
        s->path = 1;
    }
    return 1;
}

int main(void) {
    struct seen s = {0, 0, 0};
    int rc = ini_parse("sample.ini", handler, &s);
    if (rc != 0) return 20 + rc;
    return s.title && s.answer && s.path ? 0 : 2;
}
