static __attribute__((noinline)) int classify_byte(unsigned char value)
{
    if (value == ' ' || value == '\t' || value == '\n')
        return 0;
    if (value >= 'a' && value <= 'z')
        return 1;
    if (value >= '0' && value <= '9')
        return 2;
    return 3;
}

static unsigned char values[480];

unsigned int group_classes(void)
{
    unsigned int groups = 0;
    unsigned int checksum = 0;
    int previous = -1;
    unsigned int run = 0;
    int i;

    for (i = 0; i < 480; ++i) {
        int kind = classify_byte(values[i]);

        if (kind == 1 || kind == 2) {
            if (kind == previous) {
                ++run;
                continue;
            }
            if (previous >= 0) {
                ++groups;
                checksum += (unsigned int)previous * 31u + run;
            }
            previous = kind;
            run = 1;
            continue;
        }
        if (previous >= 0) {
            ++groups;
            checksum += (unsigned int)previous * 31u + run;
            previous = -1;
            run = 0;
        }
        if (kind == 3) {
            ++groups;
            checksum += values[i];
        }
    }
    if (previous >= 0) {
        ++groups;
        checksum += (unsigned int)previous * 31u + run;
    }
    return checksum * 7u + groups;
}
