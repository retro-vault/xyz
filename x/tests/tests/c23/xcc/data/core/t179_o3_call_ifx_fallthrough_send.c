extern unsigned int find(unsigned int first, unsigned int owner);
extern void sink(unsigned int value);

void probe(unsigned int owner) {
    unsigned int value = find(1u, owner);
    if (value)
        sink(value);
}
