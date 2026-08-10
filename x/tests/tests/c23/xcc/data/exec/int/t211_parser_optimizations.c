#include <ctype.h>

#include "xcc_exec_test.h"

static unsigned char token_bytes[8][4];
static unsigned int token_words[8][2];

static __attribute__((noinline)) void
copy_token_bytes(unsigned int slot, const unsigned char *text)
{
    unsigned int index;

    for (index = 0; index < 4; ++index)
        token_bytes[slot][index] = *text++;
}

static __attribute__((noinline)) void
copy_token_words(unsigned int slot, const unsigned int *text)
{
    unsigned int index;

    for (index = 0; index < 2; ++index)
        token_words[slot][index] = *text++;
}

static __attribute__((noinline)) unsigned int
count_byte(const char *text, unsigned char wanted)
{
    unsigned int count = 0;

    while (*text != '\0') {
        if ((unsigned char)*text == wanted)
            ++count;
        ++text;
    }
    return count;
}

static __attribute__((noinline)) int
keyword_equal(const char *text, const char *keyword)
{
    unsigned int index = 0;

    while (keyword[index] != '\0') {
        int actual = toupper((unsigned char)text[index]);
        int expected = toupper((unsigned char)keyword[index]);

        if (actual != expected)
            return 0;
        ++index;
    }
    return text[index] == '\0';
}

static __attribute__((noinline)) int
lookup_keyword(const char *text)
{
    if (keyword_equal(text, "SELECT"))
        return 1;
    if (keyword_equal(text, "FROM"))
        return 2;
    return 0;
}

static __attribute__((noinline)) int
keyword_missing(const char *text)
{
    return lookup_keyword(text) == 0;
}

static __attribute__((noinline)) unsigned int
count_name_chars(const char *text)
{
    unsigned int count = 0;

    while (isalnum((unsigned char)*text) || *text == '_') {
        ++count;
        ++text;
    }
    return count;
}

static __attribute__((noinline)) unsigned int
tail_count_name(const char *text)
{
    volatile unsigned int first = (unsigned char)*text;

    if (first == 0)
        return 0;
    return count_name_chars(text);
}

int
main(void)
{
    static const unsigned char first_token[4] = {'N', 'A', 'M', 'E'};
    static const unsigned char second_token[4] = {'I', 'D', '4', '2'};
    static const unsigned int word_token[2] = {0x1234u, 0xabcdU};

    XCC_CHECK_EQ_UINT_ID(1, count_byte("a,b,a", ','), 2);
    XCC_CHECK_EQ_UINT_ID(2, count_byte("a,b,a", 'a'), 2);
    XCC_CHECK_EQ_INT_ID(3, keyword_equal("select", "SELECT"), 1);
    XCC_CHECK_EQ_INT_ID(4, keyword_equal("fromx", "FROM"), 0);
    XCC_CHECK_EQ_INT_ID(5, keyword_missing("other"), 1);
    XCC_CHECK_EQ_INT_ID(6, keyword_missing("select"), 0);
    XCC_CHECK_EQ_UINT_ID(7, tail_count_name("name_12 rest"), 7);
    XCC_CHECK_EQ_UINT_ID(8, tail_count_name(""), 0);
    XCC_CHECK_EQ_INT_ID(9, toupper(-1), -1);
    XCC_CHECK_EQ_INT_ID(10, tolower(0x100), 0x100);
    XCC_CHECK_EQ_INT_ID(11, isalpha(0x141), 0);
    XCC_CHECK_EQ_INT_ID(12, isspace('\n'), 1);
    copy_token_bytes(3, first_token);
    copy_token_bytes(5, second_token);
    XCC_CHECK_EQ_UINT_ID(13, token_bytes[3][0], 'N');
    XCC_CHECK_EQ_UINT_ID(14, token_bytes[3][3], 'E');
    XCC_CHECK_EQ_UINT_ID(15, token_bytes[5][0], 'I');
    XCC_CHECK_EQ_UINT_ID(16, token_bytes[5][3], '2');
    copy_token_words(2, word_token);
    XCC_CHECK_EQ_UINT_ID(17, token_words[2][0], 0x1234u);
    XCC_CHECK_EQ_UINT_ID(18, token_words[2][1], 0xabcdu);
    return 0;
}
