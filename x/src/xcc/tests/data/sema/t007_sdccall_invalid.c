// Tests [[sdcc::sdccall(N)]] with invalid N: must produce an error.
[[sdcc::sdccall(3)]] void bad(void);

int main(void) { return 0; }
