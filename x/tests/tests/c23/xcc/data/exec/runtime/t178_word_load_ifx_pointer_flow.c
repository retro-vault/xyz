#include "xcc_exec_test.h"

static int chase_or_zero(int **pp) {
    int *p = *pp;
    if (p)
        return *p;
    return 0;
}

static const char *nonnull_or_default(const char **pp) {
    const char *p = *pp;
    if (p)
        return p;
    return "(null)";
}

int main(void) {
    int value = 7;
    int *live = &value;
    int *dead = 0;
    const char *word = "ok";
    const char *none = 0;

    XCC_CHECK_EQ_INT_ID(1, chase_or_zero(&live), 7);
    XCC_CHECK_EQ_INT_ID(2, chase_or_zero(&dead), 0);
    XCC_CHECK_EQ_INT_ID(3, nonnull_or_default(&word)[0], 'o');
    XCC_CHECK_EQ_INT_ID(4, nonnull_or_default(&none)[0], '(');
    return 0;
}
