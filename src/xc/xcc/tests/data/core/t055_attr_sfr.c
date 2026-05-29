// Tests [[sdcc::sfr(N)]]: reads compile to IN, writes to OUT.
[[sdcc::sfr(0x3F)]] unsigned char PORT_A;

void set_port(unsigned char v) { PORT_A = v; }
unsigned char get_port(void)   { return PORT_A; }
