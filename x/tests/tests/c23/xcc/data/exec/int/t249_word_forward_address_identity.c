typedef unsigned int word;
typedef unsigned char byte;

/* Both offsets stay within the caller's three-element array. */
__attribute__((noinline)) word opposite_offsets(word *middle, word value)
{
    middle[1] = value;
    return *(middle - 1);
}

/* The byte conversion wraps the offset while preserving word alignment. */
__attribute__((noinline)) word narrowed_offset(byte *base, word offset,
                                              word value)
{
    *(word *)(base + (byte)offset) = value;
    return *(word *)(base + offset);
}

/* Capturing an address does not capture subsequent changes to its source. */
__attribute__((noinline)) word captured_address(word *cursor, word value)
{
    word *saved = cursor;
    ++cursor;
    *saved = value;
    return *cursor;
}

static word distant_words[160];

int main(void)
{
    word input;
    for (input = 0; input < 256; ++input) {
        word left = (word)(0x1200u + input);
        word middle = (word)(0x3400u + input);
        word replacement = (word)(0xab00u + input);
        word neighbors[3] = {left, middle, 0x5678u};
        word near_index = input & 7u;
        word far_index = (word)(256u / sizeof(word) + near_index);
        word offset = (word)(far_index * sizeof(word));

        if (opposite_offsets(neighbors + 1, replacement) != left)
            return 1;
        if (neighbors[0] != left || neighbors[1] != middle ||
            neighbors[2] != replacement)
            return 2;

        distant_words[near_index] = left;
        distant_words[far_index] = middle;
        if (narrowed_offset((byte *)&distant_words, offset, replacement)
            != middle)
            return 3;
        if (distant_words[near_index] != replacement ||
            distant_words[far_index] != middle)
            return 4;

        neighbors[0] = left;
        neighbors[1] = middle;
        neighbors[2] = 0x5678u;
        if (captured_address(neighbors, replacement) != middle)
            return 5;
        if (neighbors[0] != replacement || neighbors[1] != middle ||
            neighbors[2] != 0x5678u)
            return 6;
    }
    return 0;
}
