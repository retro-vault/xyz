// Tests default sdcccall(1): 16-bit-return callees repair stack-passed args.
[[sdcc::sdccall(1)]] int add3(int a, int b, int c) {
    return a + b + c;
}
