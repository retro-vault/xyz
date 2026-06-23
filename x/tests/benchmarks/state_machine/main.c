#include "bench.h"

int
main(void)
{
    static bench_u8 raw[96];
    static char stream[96];
    bench_u16 acc;
    bench_u16 valid_numbers;
    bench_u16 identifiers;
    bench_u16 transitions;
    bench_u16 i;
    bench_u8 state;
    char ch;

    BENCH_FILL_ARRAY(raw, 96u, 0x22u);
    for (i = 0; i < 96u; ++i) {
        switch (raw[i] & 7u) {
        case 0: stream[i] = (char)('0' + (raw[i] & 9u)); break;
        case 1: stream[i] = (char)('a' + (raw[i] & 15u)); break;
        case 2: stream[i] = (char)('A' + (raw[i] & 7u)); break;
        case 3: stream[i] = (raw[i] & 1u) ? '+' : '-'; break;
        case 4: stream[i] = '.'; break;
        case 5: stream[i] = (raw[i] & 1u) ? 'e' : 'E'; break;
        case 6: stream[i] = ','; break;
        default: stream[i] = ' '; break;
        }
    }

    state = 0u;
    valid_numbers = 0u;
    identifiers = 0u;
    transitions = 0u;
    acc = 0x2468u;

    for (i = 0; i < 96u; ++i) {
        ch = stream[i];

        if (ch == ' ' || ch == ',') {
            if (state == 2u || state == 4u || state == 6u)
                ++valid_numbers;
            state = 0u;
            ++transitions;
            continue;
        }

        switch (state) {
        case 0:
            if (ch == '+' || ch == '-')
                state = 1u;
            else if (ch >= '0' && ch <= '9')
                state = 2u;
            else {
                state = 7u;
                ++identifiers;
            }
            ++transitions;
            break;
        case 1:
            if (ch >= '0' && ch <= '9')
                state = 2u;
            else {
                state = 7u;
                ++identifiers;
            }
            ++transitions;
            break;
        case 2:
            if (ch >= '0' && ch <= '9')
                ;
            else if (ch == '.')
                state = 4u;
            else if (ch == 'e' || ch == 'E')
                state = 5u;
            else {
                state = 7u;
                ++identifiers;
            }
            break;
        case 4:
            if (ch >= '0' && ch <= '9')
                ;
            else if (ch == 'e' || ch == 'E')
                state = 5u;
            else {
                state = 7u;
                ++identifiers;
            }
            break;
        case 5:
            if (ch == '+' || ch == '-')
                state = 3u;
            else if (ch >= '0' && ch <= '9')
                state = 6u;
            else {
                state = 7u;
                ++identifiers;
            }
            ++transitions;
            break;
        case 3:
            if (ch >= '0' && ch <= '9')
                state = 6u;
            else {
                state = 7u;
                ++identifiers;
            }
            ++transitions;
            break;
        case 6:
            if (ch >= '0' && ch <= '9')
                ;
            else {
                state = 7u;
                ++identifiers;
            }
            break;
        default:
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
                ;
            else
                state = 0u;
            break;
        }

        acc = bench_mix16(acc, (bench_u16)(bench_u8)ch);
        acc = bench_mix16(acc, state);
    }

    acc = bench_mix16(acc, valid_numbers);
    acc = bench_mix16(acc, identifiers);
    acc = bench_mix16(acc, transitions);
    return (int)acc;
}
