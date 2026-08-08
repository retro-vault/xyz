int lookup_values[73];

int
lookup_value(int wanted)
{
    int lower = 0;
    int upper = 72;

    while (lower <= upper) {
        int probe = (lower + upper) >> 1;
        int value = lookup_values[probe];

        if (value == wanted)
            return probe;
        if (value < wanted)
            lower = probe + 1;
        else
            upper = probe - 1;
    }
    return -1;
}
