/*
 * stdio_format_double.c
 *
 * Minimal floating-point formatter used by printf-family code paths.
 * It is intentionally small and self-contained: no static writable data,
 * no recursion back into stdio, and only basic %e/%E/%f/%F/%g/%G support.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#include <stddef.h>

enum {
    STDIO_FMT_CLASS_FINITE = 0,
    STDIO_FMT_CLASS_INF    = 1,
    STDIO_FMT_CLASS_NAN    = 2,
    STDIO_FMT_MAX_PREC     = 16
};

typedef union stdio_ieee_double {
    double d;
    unsigned char bytes[8];
} stdio_ieee_double;

typedef struct stdio_writer {
    char *ptr;
    size_t remaining;
    int count;
} stdio_writer;

static int stdio_is_upper_conv(unsigned conv) {
    return conv >= 'A' && conv <= 'Z';
}

static void stdio_writer_init(stdio_writer *writer, char *buffer, size_t size) {
    writer->ptr = buffer;
    writer->remaining = size;
    writer->count = 0;
}

static void stdio_writer_putc(stdio_writer *writer, char ch) {
    if (writer->remaining > 1u) {
        *writer->ptr++ = ch;
    }
    if (writer->remaining > 0u) {
        --writer->remaining;
    }
    ++writer->count;
}

static void stdio_writer_puts(stdio_writer *writer, const char *text) {
    while (*text != '\0') {
        stdio_writer_putc(writer, *text++);
    }
}

static void stdio_writer_finish(stdio_writer *writer) {
    if (writer->remaining > 0u) {
        *writer->ptr = '\0';
    }
}

static int stdio_classify_double(double value, int *negative) {
    stdio_ieee_double raw;
    unsigned exp11;
    unsigned mantissa_nonzero;

    raw.d = value;
    *negative = (raw.bytes[7] & 0x80u) != 0u;
    exp11 = (unsigned)(raw.bytes[7] & 0x7fu) << 4;
    exp11 |= (unsigned)(raw.bytes[6] >> 4);
    mantissa_nonzero = (unsigned)(raw.bytes[6] & 0x0fu);
    mantissa_nonzero |= raw.bytes[5];
    mantissa_nonzero |= raw.bytes[4];
    mantissa_nonzero |= raw.bytes[3];
    mantissa_nonzero |= raw.bytes[2];
    mantissa_nonzero |= raw.bytes[1];
    mantissa_nonzero |= raw.bytes[0];

    if (exp11 == 0x7ffu) {
        return mantissa_nonzero != 0u ? STDIO_FMT_CLASS_NAN
                                      : STDIO_FMT_CLASS_INF;
    }
    return STDIO_FMT_CLASS_FINITE;
}

static void stdio_emit_unsigned(stdio_writer *writer, unsigned long value) {
    char digits[11];
    unsigned count = 0;

    do {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u);

    while (count > 0u) {
        stdio_writer_putc(writer, digits[--count]);
    }
}

static void stdio_emit_exponent(stdio_writer *writer, char marker, int exponent) {
    unsigned magnitude;
    char digits[5];
    unsigned count = 0;

    stdio_writer_putc(writer, marker);
    if (exponent < 0) {
        stdio_writer_putc(writer, '-');
        magnitude = (unsigned)(-exponent);
    } else {
        stdio_writer_putc(writer, '+');
        magnitude = (unsigned)exponent;
    }

    do {
        digits[count++] = (char)('0' + (magnitude % 10u));
        magnitude /= 10u;
    } while (magnitude != 0u && count < sizeof(digits));

    while (count < 2u) {
        digits[count++] = '0';
    }
    while (count > 0u) {
        stdio_writer_putc(writer, digits[--count]);
    }
}

static int stdio_normalize_decimal(double *value) {
    int exponent = 0;
    int guard = 0;

    while (*value >= 10.0 && guard < 384) {
        *value /= 10.0;
        ++exponent;
        ++guard;
    }
    while (*value > 0.0 && *value < 1.0 && guard < 384) {
        *value *= 10.0;
        --exponent;
        ++guard;
    }
    return exponent;
}

static unsigned stdio_generate_digits(double value,
                                      unsigned wanted,
                                      unsigned char *digits) {
    unsigned i;

    for (i = 0; i < wanted; ++i) {
        int digit = (int)value;
        if (digit < 0) {
            digit = 0;
        } else if (digit > 9) {
            digit = 9;
        }
        digits[i] = (unsigned char)digit;
        value = (value - (double)digit) * 10.0;
        if (value < 0.0) {
            value = 0.0;
        }
    }
    return wanted;
}

static void stdio_round_digits(unsigned char *digits,
                               unsigned used,
                               int *carry_exponent) {
    int index;

    if (used == 0u) {
        return;
    }
    if (digits[used - 1u] < 5u) {
        return;
    }

    index = (int)used - 2;
    while (index >= 0) {
        if (digits[index] < 9u) {
            ++digits[index];
            return;
        }
        digits[index] = 0u;
        --index;
    }

    digits[0] = 1u;
    for (index = 1; index < (int)(used - 1u); ++index) {
        digits[index] = 0u;
    }
    ++(*carry_exponent);
}

static int stdio_format_special(stdio_writer *writer,
                                unsigned conv,
                                int negative,
                                int cls) {
    if (negative) {
        stdio_writer_putc(writer, '-');
    }
    if (cls == STDIO_FMT_CLASS_INF) {
        if (stdio_is_upper_conv(conv)) {
            stdio_writer_puts(writer, "INF");
        } else {
            stdio_writer_puts(writer, "inf");
        }
    } else {
        if (stdio_is_upper_conv(conv)) {
            stdio_writer_puts(writer, "NAN");
        } else {
            stdio_writer_puts(writer, "nan");
        }
    }
    return writer->count;
}

static int stdio_format_scientific(stdio_writer *writer,
                                   unsigned precision,
                                   unsigned conv,
                                   double value,
                                   int negative) {
    unsigned char digits[STDIO_FMT_MAX_PREC + 2];
    unsigned wanted;
    int exponent10;
    unsigned i;
    char marker;

    if (negative) {
        stdio_writer_putc(writer, '-');
        value = -value;
    }

    if (value == 0.0) {
        stdio_writer_putc(writer, '0');
        if (precision > 0u) {
            stdio_writer_putc(writer, '.');
            for (i = 0; i < precision; ++i) {
                stdio_writer_putc(writer, '0');
            }
        }
        marker = stdio_is_upper_conv(conv) ? 'E' : 'e';
        stdio_emit_exponent(writer, marker, 0);
        return writer->count;
    }

    exponent10 = stdio_normalize_decimal(&value);
    wanted = precision + 2u;
    stdio_generate_digits(value, wanted, digits);
    stdio_round_digits(digits, wanted, &exponent10);

    stdio_writer_putc(writer, (char)('0' + digits[0]));
    if (precision > 0u) {
        stdio_writer_putc(writer, '.');
        for (i = 0; i < precision; ++i) {
            stdio_writer_putc(writer, (char)('0' + digits[i + 1u]));
        }
    }

    marker = stdio_is_upper_conv(conv) ? 'E' : 'e';
    stdio_emit_exponent(writer, marker, exponent10);
    return writer->count;
}

static int stdio_format_fixed(stdio_writer *writer,
                              unsigned precision,
                              double value,
                              int negative) {
    unsigned char digits[STDIO_FMT_MAX_PREC + 1];
    unsigned long whole;
    double fraction;
    unsigned i;
    int carry = 0;

    if (negative) {
        stdio_writer_putc(writer, '-');
        value = -value;
    }

    if (value > 4294967295.0) {
        return stdio_format_scientific(writer, precision, 'e', value, 0);
    }

    whole = (unsigned long)value;
    fraction = value - (double)whole;
    if (fraction < 0.0) {
        fraction = 0.0;
    }

    if (precision > 0u) {
        stdio_generate_digits(fraction * 10.0, precision + 1u, digits);
        if (digits[precision] >= 5u) {
            int index = (int)precision - 1;
            while (index >= 0) {
                if (digits[index] < 9u) {
                    ++digits[index];
                    break;
                }
                digits[index] = 0u;
                --index;
            }
            if (index < 0) {
                carry = 1;
            }
        }
    }

    if (carry != 0) {
        ++whole;
    }
    stdio_emit_unsigned(writer, whole);

    if (precision > 0u) {
        stdio_writer_putc(writer, '.');
        for (i = 0; i < precision; ++i) {
            stdio_writer_putc(writer, (char)('0' + digits[i]));
        }
    }
    return writer->count;
}

[[sdcc::sdccall(0)]]
int stdio_format_double(char *s,
                        size_t n,
                        unsigned precision,
                        unsigned conv,
                        double value) {
    stdio_writer writer;
    int negative = 0;
    int cls;
    unsigned work_precision = precision;

    if (work_precision > STDIO_FMT_MAX_PREC) {
        work_precision = STDIO_FMT_MAX_PREC;
    }

    stdio_writer_init(&writer, s, n);
    cls = stdio_classify_double(value, &negative);
    if (cls != STDIO_FMT_CLASS_FINITE) {
        stdio_format_special(&writer, conv, negative, cls);
        stdio_writer_finish(&writer);
        return writer.count;
    }

    /* Keep the non-fixed formats on the lean scientific path so printf users
       that never print floats do not pay for extra trimming machinery. */
    if (conv == 'e' || conv == 'E' || conv == 'g' || conv == 'G' ||
        conv == 'a' || conv == 'A') {
        stdio_format_scientific(&writer, work_precision, conv, value, negative);
        stdio_writer_finish(&writer);
        return writer.count;
    }

    if (conv == 'f' || conv == 'F') {
        stdio_format_fixed(&writer, work_precision, value, negative);
        stdio_writer_finish(&writer);
        return writer.count;
    }

    stdio_format_scientific(&writer, work_precision, 'e', value, negative);
    stdio_writer_finish(&writer);
    return writer.count;
}
