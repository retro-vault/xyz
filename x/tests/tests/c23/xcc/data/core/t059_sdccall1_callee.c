// Tests default sdcccall(1): stack-passed arguments remain caller-clean.
[[sdcc::sdcccall(1)]] int add3(int a, int b, int c) {
    return a + b + c;
}
