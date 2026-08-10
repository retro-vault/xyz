int parser_keyword_equal(const char *text, const char *keyword)
{
    unsigned int index = 0;

    while (keyword[index] != 0) {
        if (text[index] != keyword[index])
            return 0;
        ++index;
    }
    return text[index] == 0;
}
