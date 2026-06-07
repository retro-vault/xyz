#include "xcc_exec_test.h"

static int is_sep(char ch) {
    return ch == ' ' || ch == ',';
}

static int is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

static int classify(char ch, unsigned state) {
    if (is_sep(ch))
        return 10;
    if ((state == 2u || state == 4u || state == 6u) && is_digit(ch))
        return 20;
    if ((ch == '+' || ch == '-') && (state == 0u || state == 5u))
        return 30;
    return 40;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, is_sep(' '), 1);
    XCC_CHECK_EQ_INT_ID(2, is_sep(','), 1);
    XCC_CHECK_EQ_INT_ID(3, is_sep('x'), 0);
    XCC_CHECK_EQ_INT_ID(4, is_digit('0'), 1);
    XCC_CHECK_EQ_INT_ID(5, is_digit('9'), 1);
    XCC_CHECK_EQ_INT_ID(6, is_digit('A'), 0);
    XCC_CHECK_EQ_INT_ID(7, classify(' ', 2u), 10);
    XCC_CHECK_EQ_INT_ID(8, classify('7', 2u), 20);
    XCC_CHECK_EQ_INT_ID(9, classify('+', 0u), 30);
    XCC_CHECK_EQ_INT_ID(10, classify('x', 7u), 40);
    return 0;
}
