// Tests [[sdcc::sfr(N)]] with a 16-bit Z80 port address.
[[sdcc::sfr(0x1234)]] unsigned char PORT_WIDE;

void set_wide(unsigned char v) { PORT_WIDE = v; }
unsigned char get_wide(void)   { return PORT_WIDE; }
