static __attribute__((noinline)) unsigned int
text_length(const char *text)
{
    const char *cursor = text;

    while (*cursor != '\0')
        ++cursor;
    return (unsigned int)(cursor - text);
}

static __attribute__((noinline)) int
text_compare(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return (int)((unsigned char)*left - (unsigned char)*right);
}

unsigned int
scan_texts(const char *text, unsigned int count)
{
    const char *previous = text;
    unsigned int total = 0;

    while (count-- != 0 && *text != '\0') {
        unsigned int length = text_length(text);
        total += length + (unsigned int)text_compare(previous, text);
        previous = text;
        text += length + 1;
    }
    return total;
}
