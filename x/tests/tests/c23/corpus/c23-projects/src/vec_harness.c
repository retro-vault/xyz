#include "vec.h"

int main(void) {
    vec_int_t v;
    vec_int_t extra;
    int i;
    int x;
    int sum = 0;

    vec_init(&v);
    vec_init(&extra);

    for (i = 0; i < 12; ++i) {
        if (vec_push(&v, i * 3) != 0) return 1;
    }
    if (v.length != 12 || v.data[0] != 0 || v.data[11] != 33) return 2;

    vec_splice(&v, 2, 3);
    if (v.length != 9 || v.data[2] != 15) return 3;

    if (vec_insert(&v, 2, 99) != 0) return 4;
    if (v.length != 10 || v.data[2] != 99 || v.data[3] != 15) return 5;

    vec_push(&extra, -4);
    vec_push(&extra, 42);
    vec_extend(&v, &extra);
    if (v.length != 12 || v.data[10] != -4 || v.data[11] != 42) return 6;

    vec_find(&v, 42, i);
    if (i < 0 || v.data[i] != 42) return 8;
    vec_remove(&v, 42);
    vec_find(&v, 42, i);
    if (i != -1) return 9;

    vec_reverse(&v);
    if (vec_first(&v) != -4 || vec_last(&v) != 0) return 12;

    vec_foreach(&v, x, i) {
        sum += x;
    }
    if (sum != 266) return 10;

    vec_truncate(&v, 4);
    if (v.length != 4) return 11;
    if (vec_compact(&v) != 0 || v.capacity != v.length) return 13;

    vec_deinit(&extra);
    vec_deinit(&v);
    return 0;
}
