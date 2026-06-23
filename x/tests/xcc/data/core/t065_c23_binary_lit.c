// C23: binary integer literals 0b... and digit separators 1'000.
int main(void) {
    int flags  = 0b10110100;
    int big    = 1'000'000;
    int masked = flags & 0b1111'0000;
    return flags + big + masked;
}
