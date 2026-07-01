// Tests [[sdcc::sdccall(1)]] register + stack parameter passing.
[[sdcc::sdccall(1)]] int add3(int a, int b, int c) {
    return a + b + c;
}
