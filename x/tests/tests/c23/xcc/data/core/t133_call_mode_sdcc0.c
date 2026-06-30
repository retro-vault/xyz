// Tests --sdcccall 0: unannotated functions default to the stack ABI.
int add(int a, int b) { return a + b; }

int main(void) { return add(1, 2); }
