// Tests default sdcccall(1): 16-bit-return callers must not pop stacked args.
[[sdcc::sdcccall(1)]] int add3(int a, int b, int c);

int main(void) {
    return add3(1, 2, 3);
}
