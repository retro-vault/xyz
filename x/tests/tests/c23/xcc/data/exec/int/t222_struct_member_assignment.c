typedef unsigned int u16;

struct record {
    u16 before;
    u16 arg;
    u16 after;
};

struct packed_record {
    unsigned int low : 3;
    unsigned int arg : 5;
    unsigned int crossing : 9;
    unsigned int high : 7;
};

static __attribute__((noinline)) u16
assign_member(struct record *item, u16 arg)
{
    item->arg = arg;
    return item->arg;
}

static __attribute__((noinline)) void
assign_bit_members(struct packed_record *item, u16 arg)
{
    item->arg = arg;
    item->crossing = (u16)(arg * 9u + 17u);
}

int
main(void)
{
    struct record item = { 0x1357u, 0x2468u, 0x9abcu };
    struct packed_record packed = { 5u, 7u, 0x155u, 0x52u };
    volatile u16 supplied = 0xbeefu;

    if (assign_member(&item, supplied) != 0xbeefu)
        return 1;
    if (item.before != 0x1357u || item.arg != 0xbeefu ||
        item.after != 0x9abcu)
        return 2;

    assign_bit_members(&packed, supplied);
    if (packed.low != 5u || packed.arg != 15u ||
        packed.crossing != 0x78u || packed.high != 0x52u)
        return 3;
    return 0;
}
