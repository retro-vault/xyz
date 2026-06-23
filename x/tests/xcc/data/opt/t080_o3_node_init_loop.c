typedef unsigned char u8;

struct node {
    u8 key;
    u8 next;
};

static struct node nodes[24];
static u8 keys[24] = {
    0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u,
    8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u,
    16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u
};

int
main(void)
{
    u8 idx;

    for (idx = 0; idx < 24u; ++idx) {
        nodes[idx].key = keys[idx];
        nodes[idx].next = 0xffu;
    }

    return (int)nodes[0].next;
}
