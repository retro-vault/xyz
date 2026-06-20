#include "kvec.h"

int main(void) {
    kvec_t(int) v;
    int sum;

    kv_init(v);
    kv_push(int, v, 7);
    kv_push(int, v, 11);
    kv_push(int, v, 23);
    sum = kv_A(v, 0) + kv_A(v, 1) + kv_A(v, 2);
    sum = (kv_size(v) == 3 && sum == 41);
    kv_destroy(v);
    return sum ? 0 : 1;
}
