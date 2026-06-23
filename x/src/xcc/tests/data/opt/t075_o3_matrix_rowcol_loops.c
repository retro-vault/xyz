static unsigned char src[64];
static unsigned char row_sum[8];
static unsigned char col_sum[8];

int main(void) {
    unsigned char r;
    unsigned char c;
    unsigned char acc;

    for (r = 0; r < 8u; ++r) {
        row_sum[r] = 0u;
        col_sum[r] = 0u;
    }

    for (r = 0; r < 8u; ++r) {
        for (c = 0; c < 8u; ++c) {
            unsigned char idx = (unsigned char)(r * 8u + c);
            row_sum[r] = (unsigned char)(row_sum[r] + src[idx]);
            col_sum[c] = (unsigned char)(col_sum[c] + src[idx]);
        }
    }

    acc = 0u;
    for (r = 0; r < 8u; ++r)
        acc = (unsigned char)(acc + (unsigned char)(row_sum[r] ^ col_sum[r]));
    return (int)acc;
}
