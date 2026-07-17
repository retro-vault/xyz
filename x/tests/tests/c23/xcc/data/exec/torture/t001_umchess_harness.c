#include <stdint.h>

extern long M, S, I, C, Q, O, K, N;
extern char L, *P, w[], o[], b[], n[], c[];

long D(long k, long q, long l, long e, long E, long z, long depth);

static uint32_t board_hash(void)
{
    uint32_t hash = UINT32_C(2166136261);
    int square;

    for (square = 0; square < 129; ++square)
        hash = (hash ^ (unsigned char)b[square]) * UINT32_C(16777619);
    return hash;
}

static void setup_initial_board(void)
{
    K = 8;
    while (K--) {
        b[K] = (b[K + 112] = o[K + 24] + 8) + 8;
        b[K + 16] = 18;
        b[K + 96] = 9;
        L = 8;
        while (L--) {
            int rank_distance = 2 * L - 7;
            b[16 * L + K + 8] = (K - 4) * (K - 4) +
                rank_distance * rank_distance / 4;
        }
    }
}

int main(void)
{
    long result;

    setup_initial_board();
    if (K != -1 || L != -1 || board_hash() != UINT32_C(4097618343))
        return 1;
    if (o[0] != -16 || o[1] != -15 || o[2] != -17)
        return 6;

    /* Exercise the same recursive engine with a bounded non-root search.
     * z=8 requests iterative deepening until a complete computer move and is
     * intentionally interactive-scale; z=64, depth=1 retains move generation,
     * evaluation, make/unmake, and the recapture path while giving the
     * execution suite a deterministic instruction budget. */
    P = c;
    N = -1;
    K = I;
    result = D(8, -I, I, Q, O, 64, 1);

    if (result != 6)
        return 2;
    if (K != I || L != -1)
        return 3;
    if (N != 1 || Q != 0 || O != 0)
        return 4;
    if (board_hash() != UINT32_C(3929842153))
        return 5;
    return 0;
}
