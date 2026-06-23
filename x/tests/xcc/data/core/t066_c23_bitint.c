// C23: _BitInt(N) bit-precise integer type.
_BitInt(7)          small  = 0;
unsigned _BitInt(16) port  = 0xFFFF;
_BitInt(32)          large = 0;

int get_port(void) { return (int)port; }
