#include <string.h>

#include "ini.h"

int main(void) {
    ini_t *ini = ini_load("settings.ini");
    const char *title;
    const char *answer;
    const char *quoted;

    if (!ini) return 1;

    title = ini_get(ini, "main", "title");
    answer = ini_get(ini, "main", "answer");
    quoted = ini_get(ini, "main", "quoted");

    if (!title || strcmp(title, "X Tools") != 0) return 2;
    if (!answer || strcmp(answer, "42") != 0) return 3;
    if (!quoted || strcmp(quoted, "z80\tc23") != 0) return 4;
    if (ini_get(ini, "missing", "title") != 0) return 5;

    ini_free(ini);
    return 0;
}
