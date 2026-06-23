// C23: typeof_unqual strips const/volatile from the deduced type.
const int x = 42;
typeof_unqual(x) y = 0;  // y is non-const int

int get(void) { y = 1; return y; }
