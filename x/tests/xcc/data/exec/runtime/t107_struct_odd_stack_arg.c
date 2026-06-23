#include "xcc_exec_test.h"

struct Tiny {
    unsigned char tag;
    unsigned short ptr;
};

struct RegexLike {
    unsigned char type;
    union {
        unsigned char ch;
        unsigned char *ccl;
    } u;
};

static int check_tiny(struct Tiny t, unsigned char c) {
    if (t.tag != 0x34)
        return 10 + t.tag;
    if (t.ptr != 0x7856)
        return 20;
    return c == 0x9a ? 123 : 30 + c;
}

static int check_regex_like(struct RegexLike r, unsigned char c) {
    switch (r.type) {
    case 8:
        return r.u.ccl[0] == c && r.u.ccl[1] == 'h';
    default:
        return 0;
    }
}

int main(void) {
    unsigned char klass[3];
    struct Tiny t;
    struct RegexLike r;

    klass[0] = 'H';
    klass[1] = 'h';
    klass[2] = 0;
    t.tag = 0x34;
    t.ptr = 0x7856;
    r.type = 8;
    r.u.ccl = klass;

    XCC_CHECK_EQ_INT_ID(1, sizeof(t), 3);
    XCC_CHECK_EQ_INT_ID(2, check_tiny(t, 0x9a), 123);
    XCC_CHECK_EQ_INT_ID(3, sizeof(r), 3);
    XCC_CHECK_EQ_INT_ID(4, check_regex_like(r, 'H'), 1);
    return 0;
}
