extern int printf(const char *format, ...);
extern int scanf(const char *format, ...);

int dynamic_formats(const char *output_format, const char *input_format,
                    int value)
{
    printf(output_format, value);
    return scanf(input_format, &value);
}
