int printf(const char *fmt, ...);

int main(void) {
    long whole = -123456L;
    long fraction = 345L;

    return printf("%ld.%03ld\n", whole, fraction);
}
