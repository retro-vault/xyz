// Tests [[sdcc::sdccall(1)]] callee: params arrive in HL/DE/BC, spilled to frame.
[[sdcc::sdccall(1)]] int add3(int a, int b, int c) {
    return a + b + c;
}
