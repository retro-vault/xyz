#include "re.h"

static int must_match(const char *pattern, const char *text, int expected_len) {
    int len = 0;
    int pos = re_match(pattern, text, &len);
    return pos >= 0 && len == expected_len;
}

static int must_not_match(const char *pattern, const char *text) {
    int len = 0;
    return re_match(pattern, text, &len) < 0;
}

int main(void) {
    if (!must_match("[Hh]ello [Ww]orld\\s*[!]?", "Hello world !", 12)) return 1;
    if (!must_match("\\d\\d?:\\d\\d?:\\d\\d?", "00:00:00", 7)) return 2;
    if (!must_match("[a-z]+\nbreak", "blahblah\nbreak", 14)) return 3;
    if (!must_not_match("\\d\\d:\\d\\d:\\d\\d", "0s:00:00")) return 4;
    return 0;
}
