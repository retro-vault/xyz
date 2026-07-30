#include <stdint.h>

static uint64_t rolling_hash(const unsigned char *text)
{
    uint64_t hash = UINT64_C(0xcbf29ce484222325);

    while (*text != 0) {
        hash ^= *text++;
        hash *= UINT64_C(0x100000001b3);
    }
    return hash;
}

int main(void)
{
    static const unsigned char input[] =
        "allocator-wide-helper-clobber";
    return rolling_hash(input) == UINT64_C(0xca698739c9cebcc3) ? 0 : 1;
}
