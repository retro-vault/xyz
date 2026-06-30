// Tests -O2: extern inline still emits an external definition when unused.
extern inline int helper(int x) { return x + 1; }

int main(void) { return 0; }
