// Tests [[sdcc::sdccall(0)]]: explicit stack ABI, same as default.
[[sdcc::sdccall(0)]] int add(int a, int b) { return a + b; }

int main(void) { return add(1, 2); }
