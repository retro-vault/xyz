#include "bench.h"

int
main(void)
{
    static bench_u8 ring[16];
    static bench_u8 stream[96];
    bench_u8 head;
    bench_u8 tail;
    bench_u8 count;
    bench_u8 i;
    bench_u16 acc;

    BENCH_FILL_ARRAY(stream, 96u, 0xc3u);
    head = 0u;
    tail = 0u;
    count = 0u;
    acc = 0x89abu;

    for (i = 0; i < 96u; ++i) {
        if (count != 16u) {
            ring[head] = (bench_u8)(stream[i] ^ (bench_u8)(i << 1));
            head = (bench_u8)((head + 1u) & 15u);
            ++count;
        }

        if ((stream[i] & 1u) != 0u && count != 0u) {
            acc = bench_mix16(acc, ring[tail]);
            tail = (bench_u8)((tail + 1u) & 15u);
            --count;
        }
    }

    while (count != 0u) {
        acc = bench_mix16(acc, ring[tail]);
        tail = (bench_u8)((tail + 1u) & 15u);
        --count;
    }

    return (int)acc;
}
