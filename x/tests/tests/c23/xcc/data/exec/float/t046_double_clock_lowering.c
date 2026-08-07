#include "xcc_exec_test.h"

static const double samples[] = {
    -0.5,
    -(0.25 + 0.25),
    0.5
};

static const double scalar_sample = -(0.25 + 0.25);

double clock_sample(int index) {
    return samples[index];
}

int clock_coord(int value) {
    return value;
}

int main(void) {
    double radius = 200.0;
    int from_literal = samples[0] * radius;
    int from_expression = samples[1] * radius;
    int from_scalar = scalar_sample * radius;
    int y = -clock_sample(2) * radius;
    int coord = clock_coord(
        512.0 + clock_sample(2) * radius - 6.0);

    XCC_CHECK_EQ_INT_ID(1, from_literal, -100);
    XCC_CHECK_EQ_INT_ID(2, from_expression, -100);
    XCC_CHECK_EQ_INT_ID(3, from_scalar, -100);
    XCC_CHECK_EQ_INT_ID(4, y, -100);
    XCC_CHECK_EQ_INT_ID(5, coord, 606);
    return 0;
}
