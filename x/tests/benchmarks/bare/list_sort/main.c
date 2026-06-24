#include "bench.h"

struct node {
    bench_u8 key;
    bench_u8 next;
};

int
main(void)
{
    static struct node nodes[24];
    static bench_u8 keys[24];
    bench_u8 head;
    bench_u8 idx;
    bench_u8 cur;
    bench_u8 prev;
    bench_u16 acc;

    BENCH_FILL_ARRAY(keys, 24u, 0x33u);
    for (idx = 0; idx < 24u; ++idx) {
        nodes[idx].key = keys[idx];
        nodes[idx].next = 0xffu;
    }

    head = 0xffu;
    for (idx = 0; idx < 24u; ++idx) {
        prev = 0xffu;
        cur = head;
        while (cur != 0xffu && nodes[cur].key <= nodes[idx].key) {
            prev = cur;
            cur = nodes[cur].next;
        }

        if (prev == 0xffu) {
            nodes[idx].next = head;
            head = idx;
        } else {
            nodes[idx].next = nodes[prev].next;
            nodes[prev].next = idx;
        }
    }

    acc = 0x3456u;
    idx = head;
    while (idx != 0xffu) {
        acc = bench_mix16(acc, nodes[idx].key);
        acc = bench_mix16(acc, idx);
        idx = nodes[idx].next;
    }

    return (int)acc;
}
