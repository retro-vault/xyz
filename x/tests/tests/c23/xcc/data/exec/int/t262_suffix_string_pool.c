/* Literal suffixes may share storage; mutable arrays must remain distinct. */
const char *narrow[] = {
    "alphabet", "bet", "", "a\0tail", "tail", "a\0other", "other"
};
const unsigned short *words[] = {
    u"alphabet", u"bet", u"", u"a\0tail", u"tail", u"a\0other", u"other"
};
const unsigned long *dwords[] = {
    U"alphabet", U"bet", U"", U"a\0tail", U"tail", U"a\0other", U"other"
};
const unsigned char *utf8[] = {
    (const unsigned char *)u8"alphabet", (const unsigned char *)u8"bet",
    (const unsigned char *)u8"", (const unsigned char *)u8"\u00e9suffix",
    (const unsigned char *)u8"suffix"
};
char mutable_one[] = "alphabet";
char mutable_two[] = "bet";
const char *relocated = "alphabet" + 5;

#ifndef POOL_DATA_ONLY
static const unsigned char expected[7][9] = {
    {'a','l','p','h','a','b','e','t',0},
    {'b','e','t',0},
    {0},
    {'a',0,'t','a','i','l',0},
    {'t','a','i','l',0},
    {'a',0,'o','t','h','e','r',0},
    {'o','t','h','e','r',0}
};
static const unsigned lengths[7] = {9, 4, 1, 7, 5, 8, 6};

int main(void) {
    unsigned i, j;
    for (i = 0; i != 7; ++i) {
        for (j = 0; j != lengths[i]; ++j) {
            if ((unsigned char)narrow[i][j] != expected[i][j]) return 1;
            if (words[i][j] != expected[i][j]) return 2;
            if (dwords[i][j] != expected[i][j]) return 3;
        }
    }
    mutable_one[5] = 'X';
    mutable_two[0] = 'Y';
    if (narrow[0][5] != 'b' || narrow[1][0] != 'b') return 4;
    if (mutable_one[5] != 'X' || mutable_two[0] != 'Y') return 5;
    if (relocated[0] != 'b' || relocated[1] != 'e' ||
        relocated[2] != 't' || relocated[3] != 0) return 6;
    if (utf8[0][5] != 'b' || utf8[1][1] != 'e' || utf8[2][0]) return 7;
    if (utf8[3][0] != 0xc3 || utf8[3][1] != 0xa9 ||
        utf8[3][2] != 's' || utf8[3][8] != 0 ||
        utf8[4][0] != 's' || utf8[4][5] != 'x') return 8;
    return 0;
}
#endif
