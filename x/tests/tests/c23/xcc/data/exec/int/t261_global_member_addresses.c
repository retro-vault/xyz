struct collection {
    unsigned count;
    unsigned keys[37];
    unsigned items[37];
};

struct collection objects;

__attribute__((noinline)) void append(unsigned key, unsigned item)
{
    unsigned index = objects.count++;
    objects.keys[index] = key;
    objects.items[index] = item;
}

#ifndef ADDRESS_CORE_ONLY
int main(void)
{
    for (unsigned i = 0; i < 37; ++i) {
        objects.keys[i] = 0xaaaa;
        objects.items[i] = 0x5555;
    }
    for (unsigned i = 0; i < 37; ++i) {
        append(0xffd0u + i, 0x1234u - i);
        if (objects.count != i + 1) return 1;
        if (objects.keys[i] != 0xffd0u + i) return 2;
        if (objects.items[i] != 0x1234u - i) return 3;
        if (i + 1 < 37 && (objects.keys[i + 1] != 0xaaaa ||
                          objects.items[i + 1] != 0x5555)) return 4;
    }
    for (unsigned i = 0; i < 37; ++i) {
        if (objects.keys[i] != 0xffd0u + i) return 5;
        if (objects.items[i] != 0x1234u - i) return 6;
    }
    return 0;
}
#endif
