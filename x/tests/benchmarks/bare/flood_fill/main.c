#include "bench.h"

int
main(void)
{
    static bench_u8 cells[144];
    static bench_u8 seen[144];
    static bench_u8 queue_r[144];
    static bench_u8 queue_c[144];
    bench_u8 head;
    bench_u8 tail;
    bench_u8 row;
    bench_u8 col;
    bench_u8 row_base;
    bench_u8 pos;
    bench_u16 acc;
    bench_u8 i;

    BENCH_FILL_ARRAY(cells, 144u, 0x17u);
    for (i = 0u; i < 144u; ++i) {
        cells[i] = (bench_u8)((cells[i] >> 5) & 1u);
        seen[i] = 0u;
    }

    pos = bench_seed_byte(0x29u);
    while (pos >= 144u)
        pos = (bench_u8)(pos - 144u);
    cells[pos] = 0u;

    row = 0u;
    row_base = 0u;
    while ((bench_u8)(row_base + 12u) <= pos) {
        row_base = (bench_u8)(row_base + 12u);
        ++row;
    }
    col = (bench_u8)(pos - row_base);

    head = 0u;
    tail = 0u;
    queue_r[tail] = row;
    queue_c[tail] = col;
    ++tail;
    seen[pos] = 1u;
    acc = 0xabcd;

    while (head != tail) {
        row = queue_r[head];
        col = queue_c[head];
        ++head;
        row_base = (bench_u8)(row << 3);
        row_base = (bench_u8)(row_base + (bench_u8)(row << 2));
        pos = (bench_u8)(row_base + col);
        acc = bench_mix16(acc, pos);

        if (col != 0u) {
            i = (bench_u8)(pos - 1u);
            if (cells[i] == 0u && seen[i] == 0u) {
                seen[i] = 1u;
                queue_r[tail] = row;
                queue_c[tail] = (bench_u8)(col - 1u);
                ++tail;
            }
        }
        if (col != 11u) {
            i = (bench_u8)(pos + 1u);
            if (cells[i] == 0u && seen[i] == 0u) {
                seen[i] = 1u;
                queue_r[tail] = row;
                queue_c[tail] = (bench_u8)(col + 1u);
                ++tail;
            }
        }
        if (row != 0u) {
            i = (bench_u8)(pos - 12u);
            if (cells[i] == 0u && seen[i] == 0u) {
                seen[i] = 1u;
                queue_r[tail] = (bench_u8)(row - 1u);
                queue_c[tail] = col;
                ++tail;
            }
        }
        if (row != 11u) {
            i = (bench_u8)(pos + 12u);
            if (cells[i] == 0u && seen[i] == 0u) {
                seen[i] = 1u;
                queue_r[tail] = (bench_u8)(row + 1u);
                queue_c[tail] = col;
                ++tail;
            }
        }
    }

    for (i = 0u; i < 144u; ++i)
        acc = bench_mix16(acc, seen[i]);
    return (int)acc;
}
