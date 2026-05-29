// Tests [[sdcc::at(N)]]: global variable becomes an absolute symbol.
[[sdcc::at(0x4000)]] int mapped_var;

int get(void) { return mapped_var; }
