extern int parser_lookup(const char *text);

int parser_missing(const char *text)
{
    return parser_lookup(text) == 0;
}

int parser_present(const char *text)
{
    return 0 != parser_lookup(text);
}
