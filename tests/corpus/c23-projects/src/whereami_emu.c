#include <string.h>

#include "whereami.h"

static const char emu_path[] = "/emu/corpus/whereami.bin";

static int copy_path(char *out, int capacity, int *dirname_length) {
    int length = (int)strlen(emu_path);
    int i;

    if (capacity >= length && out != 0) {
        memcpy(out, emu_path, length);
        if (dirname_length != 0) {
            *dirname_length = 0;
            for (i = length - 1; i >= 0; --i) {
                if (out[i] == '/') {
                    *dirname_length = i;
                    break;
                }
            }
        }
    }

    return length;
}

int wai_getExecutablePath(char *out, int capacity, int *dirname_length) {
    return copy_path(out, capacity, dirname_length);
}

int wai_getModulePath(char *out, int capacity, int *dirname_length) {
    return copy_path(out, capacity, dirname_length);
}
