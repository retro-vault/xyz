static int parser_count(const char *text, unsigned char wanted)
{
    int count = 0;

    while (*text) {
        if ((unsigned char)*text == wanted)
            ++count;
        ++text;
    }
    return count;
}

int parser_count_twice(const char *text, unsigned char wanted)
{
    return parser_count(text, wanted) + parser_count(text + 1, wanted);
}
