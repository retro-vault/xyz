#include <string.h>

#include "utf8.h"

int main(void) {
    utf8_int8_t text[] = {
        'c', 'a', 'f', (char)0xc3, (char)0xa9, ' ', 'x', 0
    };
    utf8_int8_t copy[16];
    utf8_int32_t cp = 0;
    utf8_int8_t *next;

    if (utf8len(text) != 6) return 1;
    if (utf8size(text) != 8) return 2;
    if (utf8valid(text) != 0) return 3;
    if (utf8nlen(text, 5) != 4) return 4;
    if (utf8casecmp("XTOOLS", "xtools") != 0) return 5;

    utf8cpy(copy, "z80");
    utf8cat(copy, "-c23");
    if (strcmp(copy, "z80-c23") != 0) return 6;

    next = utf8codepoint(text + 3, &cp);
    if (cp != 0x00e9 || next != text + 5) return 7;
    if (utf8codepointsize(cp) != 2) return 8;

    if (utf8chr(text, 0x00e9) != text + 3) return 9;
    return 0;
}
