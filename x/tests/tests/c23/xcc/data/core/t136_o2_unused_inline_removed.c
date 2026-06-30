// Tests -O2: an unused plain inline definition is dropped from output.
inline int helper(int x) { return x + 1; }

int main(void) { return 0; }
