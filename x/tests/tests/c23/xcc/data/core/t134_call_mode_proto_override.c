// Tests --sdcccall 0 with a prototype attribute overriding the default.
[[sdcc::sdccall(1)]] int add(int a, int b);

int add(int a, int b) { return a + b; }

int plain(int x) { return x; }

int main(void) { return add(plain(1), 2); }
