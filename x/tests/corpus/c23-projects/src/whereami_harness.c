#include <string.h>

#include "whereami.h"

int main(void) {
    char path[40];
    int dirname_length = -1;
    int length;

    length = wai_getExecutablePath(0, 0, 0);
    if (length != 24) return 1;

    if (wai_getExecutablePath(path, sizeof(path), &dirname_length) != length) {
        return 2;
    }
    path[length] = 0;
    if (strcmp(path, "/emu/corpus/whereami.bin") != 0) return 3;
    if (dirname_length != 11) return 4;

    if (wai_getModulePath(path, sizeof(path), 0) != length) return 5;
    path[length] = 0;
    return strcmp(path, "/emu/corpus/whereami.bin") == 0 ? 0 : 6;
}
