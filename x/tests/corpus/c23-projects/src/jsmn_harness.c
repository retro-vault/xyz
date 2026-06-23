#include <string.h>

#define JSMN_STATIC
#include "jsmn.h"

static int tok_streq(const char *json, const jsmntok_t *tok, const char *s) {
    int len = tok->end - tok->start;
    return len == (int)strlen(s) && strncmp(json + tok->start, s, len) == 0;
}

int main(void) {
    const char *json = "{\"name\":\"xcc\",\"ports\":[63,4660],\"ok\":true}";
    jsmn_parser parser;
    jsmntok_t tok[16];
    int n;

    jsmn_init(&parser);
    n = jsmn_parse(&parser, json, strlen(json), tok, 16);
    if (n != 9) return 1;
    if (tok[0].type != JSMN_OBJECT || tok[0].size != 3) return 2;
    if (!tok_streq(json, &tok[1], "name")) return 3;
    if (!tok_streq(json, &tok[2], "xcc")) return 4;
    if (!tok_streq(json, &tok[3], "ports")) return 5;
    if (tok[4].type != JSMN_ARRAY || tok[4].size != 2) return 6;
    if (!tok_streq(json, &tok[5], "63")) return 7;
    if (!tok_streq(json, &tok[6], "4660")) return 8;
    if (!tok_streq(json, &tok[7], "ok")) return 9;
    if (!tok_streq(json, &tok[8], "true")) return 10;

    jsmn_init(&parser);
    n = jsmn_parse(&parser, json, 10, tok, 16);
    return n == JSMN_ERROR_PART ? 0 : 11;
}
