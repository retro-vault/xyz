#include "bench.h"

int
main(void)
{
    static bench_u8 buf[96];
    bench_u16 crc;
    bench_u16 i;
    bench_u8 bit;

    BENCH_FILL_ARRAY(buf, 96u, 0x11u);

    crc = (bench_u16)(0x1021u ^ bench_seed_word());
    for (i = 0; i < 96u; ++i) {
        crc ^= (bench_u16)buf[i] << 8;
        for (bit = 0; bit < 8u; ++bit) {
            if (crc & 0x8000u)
                crc = (bench_u16)((crc << 1) ^ 0x1021u);
            else
                crc = (bench_u16)(crc << 1);
        }
        crc = bench_mix16(crc, buf[i]);
    }

    return (int)crc;
}
