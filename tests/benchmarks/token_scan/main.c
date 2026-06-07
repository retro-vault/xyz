#include "bench.h"

int
main(void)
{
    static bench_u8 raw[120];
    static char text[120];
    bench_u16 acc;
    bench_u16 token_len;
    bench_u16 token_hash;
    bench_u8 in_token;
    bench_u16 i;
    char ch;

    BENCH_FILL_ARRAY(raw, 120u, 0xd4u);
    for (i = 0; i < 120u; ++i) {
        switch (raw[i] & 7u) {
        case 0: text[i] = (char)('a' + (raw[i] & 15u)); break;
        case 1: text[i] = (char)('A' + (raw[i] & 15u)); break;
        case 2: text[i] = (char)('0' + (raw[i] & 7u)); break;
        case 3: text[i] = '_'; break;
        case 4: text[i] = '-'; break;
        case 5: text[i] = ':'; break;
        case 6: text[i] = ' '; break;
        default: text[i] = ','; break;
        }
    }

    acc = 0x9abcu;
    token_len = 0u;
    token_hash = 0u;
    in_token = 0u;

    for (i = 0; i < 120u; ++i) {
        ch = text[i];
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_') {
            in_token = 1u;
            ++token_len;
            token_hash = bench_mix16(token_hash, (bench_u16)(bench_u8)ch);
        } else if (in_token) {
            acc = bench_mix16(acc, token_len);
            acc = bench_mix16(acc, token_hash);
            token_len = 0u;
            token_hash = 0u;
            in_token = 0u;
        }
    }

    if (in_token) {
        acc = bench_mix16(acc, token_len);
        acc = bench_mix16(acc, token_hash);
    }

    return (int)acc;
}
