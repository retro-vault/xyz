// Tests rule_ld_a_zero: ld a,0 -> xor a for 8-bit zero stores
char g;
void f(void) { g = 0; }
