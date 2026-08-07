#include "xcc_exec_test.h"

static const float samples[] = {
    -0.5f,
    -(0.25f + 0.25f),
    0.5f
};

static const float scalar_sample = -(0.25f + 0.25f);

float clock_sample(int index) {
    return samples[index];
}

int clock_coord(int value) {
    return value;
}

int main(void) {
    float radius = 200.0f;
    int y = -clock_sample(2) * radius;
    int coord = clock_coord(
        512.0f + clock_sample(2) * radius - 6.0f);

    XCC_CHECK_EQ_INT_ID(1, clock_sample(0) * radius, -100);
    XCC_CHECK_EQ_INT_ID(2, clock_sample(1) * radius, -100);
    XCC_CHECK_EQ_INT_ID(3, scalar_sample * radius, -100);
    XCC_CHECK_EQ_INT_ID(4, y, -100);
    XCC_CHECK_EQ_INT_ID(5, coord, 606);
    return 0;
}
