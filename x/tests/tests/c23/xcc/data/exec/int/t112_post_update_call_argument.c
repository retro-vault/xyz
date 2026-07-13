#include "xcc_exec_test.h"

struct output_context {
    volatile unsigned char *output;
    unsigned int count;
};

static volatile unsigned char captured[8];

void
capture_character(int value, void *opaque)
{
    struct output_context *context = (struct output_context *)opaque;

    context->output[context->count++] = (unsigned char)value;
}

unsigned int
capture_literal(const char *input)
{
    struct output_context context;
    const char *cursor = input;

    context.output = captured;
    context.count = 0u;
    while (*cursor)
        capture_character((int)*cursor++, &context);
    return context.count;
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, capture_literal("z80"), 3u);
    XCC_CHECK_EQ_UINT_ID(2, captured[0], 'z');
    XCC_CHECK_EQ_UINT_ID(3, captured[1], '8');
    XCC_CHECK_EQ_UINT_ID(4, captured[2], '0');
    return 0;
}
