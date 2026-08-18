extern int printf(const char *format, ...);
extern int sscanf(const char *input, const char *format, ...);

int format_metadata(const char *text, long value, long long wide)
{
    unsigned long parsed;
    short small;
    char word[4];

    printf("%s:%04ld:%c", text, value, 'x');
    printf("%lld:%lli", wide, wide);
    return sscanf(text, "%3hd %lu %[^x]", &small, &parsed, word);
}
