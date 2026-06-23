typedef unsigned char u8;
typedef unsigned int u16;

struct node {
    u8 key;
    u8 next;
};

static u16
bench_mix16(u16 acc, u16 value)
{
    acc ^= (u16)(value + 0x9e37u);
    acc = (u16)((acc << 5) | (acc >> 11));
    acc += (u16)(value ^ 0x7f4au);
    return acc;
}

int
main(void)
{
    static struct node nodes[4] = {
        { 7u, 2u },
        { 3u, 0xffu },
        { 9u, 1u },
        { 1u, 0xffu },
    };
    u8 idx;
    u16 acc;

    acc = 0x3456u;
    idx = 0u;
    while (idx != 0xffu) {
        acc = bench_mix16(acc, nodes[idx].key);
        acc = bench_mix16(acc, idx);
        idx = nodes[idx].next;
    }

    return (int)acc;
}
