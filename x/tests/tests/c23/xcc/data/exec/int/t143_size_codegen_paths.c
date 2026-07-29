#include "xcc_exec_test.h"

struct counters {
    unsigned reserved;
    unsigned total;
};

static struct counters counts;
static unsigned first_value;
static unsigned second_value;
static unsigned callback_count;
static volatile unsigned consume_bias;
static void (*hook)(void);
static char selected_a[] = "A";
static char selected_b[] = "B";
static unsigned char cursor_values[] = { 2, 3, 5, 7, 11 };
static unsigned char fold_values[] = { 1, 2, 3, 4 };
static unsigned char equal_values[] = { 1, 2, 3, 4 };
static unsigned char different_values[] = { 1, 2, 3, 5 };

__attribute__((noinline)) static void
count_callback(void)
{
    ++callback_count;
}

__attribute__((noinline)) static void
run_hook(void)
{
    if (hook)
        hook();
}

__attribute__((noinline)) static unsigned
classify(volatile int *state)
{
    if (*state == 1)
        return 11;
    if (*state == 2)
        return 22;
    if (*state == 3)
        return 33;
    return 44;
}

__attribute__((noinline)) static unsigned
consume_selected(unsigned a, unsigned b, unsigned c, unsigned d,
                 const char *selected)
{
    return a + b + c + d + (unsigned)selected[0] + consume_bias;
}

__attribute__((noinline)) static unsigned
select_and_send(volatile int *state)
{
    return consume_selected(1, 2, 3, 4,
                            *state == 3 ? selected_a : selected_b);
}

__attribute__((noinline)) static unsigned
sum_arguments(unsigned a, unsigned b, unsigned c, unsigned d)
{
    return a + b + c + d;
}

__attribute__((noinline)) static unsigned
send_global_member(void)
{
    return sum_arguments(counts.total, first_value, second_value, 5);
}

__attribute__((noinline)) static unsigned
sum_cursor(unsigned char *cursor, unsigned count)
{
    unsigned total = 0;

    while (count--)
        total += *cursor++;
    return total;
}

__attribute__((noinline)) static void
advance_alias(unsigned char **cursor)
{
    ++*cursor;
}

__attribute__((noinline)) static unsigned
sum_aliased_cursor(unsigned char *cursor)
{
    unsigned first = *cursor;

    advance_alias(&cursor);
    return first + *cursor;
}

__attribute__((noinline)) static unsigned
fold_four(const unsigned char *values)
{
    unsigned result = 5381u;
    int i;

    for (i = 0; i < 4; ++i)
        result = (result << 5) + result + values[i];
    return result;
}

__attribute__((noinline)) static int
equal_four(const unsigned char *left, const unsigned char *right)
{
    int i;

    for (i = 0; i < 4; ++i) {
        if (left[i] != right[i])
            return 0;
    }
    return 1;
}

__attribute__((noinline)) unsigned
mix_many(unsigned value)
{
    int i;

    for (i = 0; i < 300; ++i)
        value = value * 25173u + 13849u;
    return value;
}

int
main(void)
{
    volatile int state = 1;

    XCC_CHECK_EQ_UINT_ID(1, classify(&state), 11u);
    state = 2;
    XCC_CHECK_EQ_UINT_ID(2, classify(&state), 22u);
    state = 3;
    XCC_CHECK_EQ_UINT_ID(3, classify(&state), 33u);
    state = 4;
    XCC_CHECK_EQ_UINT_ID(4, classify(&state), 44u);

    hook = 0;
    run_hook();
    XCC_CHECK_EQ_UINT_ID(5, callback_count, 0u);
    hook = count_callback;
    run_hook();
    run_hook();
    XCC_CHECK_EQ_UINT_ID(6, callback_count, 2u);

    state = 3;
    XCC_CHECK_EQ_UINT_ID(7, select_and_send(&state), 75u);
    state = 2;
    XCC_CHECK_EQ_UINT_ID(8, select_and_send(&state), 76u);

    counts.total = 7;
    first_value = 9;
    second_value = 11;
    XCC_CHECK_EQ_UINT_ID(9, send_global_member(), 32u);
    XCC_CHECK_EQ_UINT_ID(10, sum_cursor(cursor_values, 5), 28u);
    XCC_CHECK_EQ_UINT_ID(11, sum_aliased_cursor(cursor_values), 5u);
    XCC_CHECK_EQ_UINT_ID(12, fold_four(fold_values), 42191u);
    XCC_CHECK_EQ_UINT_ID(13, equal_four(fold_values, equal_values), 1u);
    XCC_CHECK_EQ_UINT_ID(14, equal_four(fold_values, different_values), 0u);
    XCC_CHECK_EQ_UINT_ID(15, mix_many(1234u), 34534u);
    XCC_CHECK_EQ_UINT_ID(16, fold_four(equal_values), 42191u);
    XCC_CHECK_EQ_UINT_ID(17, mix_many(1234u), 34534u);
    return 0;
}
