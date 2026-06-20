// SFR ports are Z80 I/O addresses, so they must fit in 16 bits.
[[sdcc::sfr(0x10000)]] unsigned char PORT_TOO_WIDE;
