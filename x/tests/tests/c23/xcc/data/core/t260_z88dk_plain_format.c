extern int printf(const char *format, ...);
extern int sscanf(const char *input, const char *format, ...);

int plain_formats(const char *text)
{
    printf("plain text");
    return sscanf(text, "plain text");
}
