// Tests sema: wrong number of arguments to a function with a known prototype.
int add(int a, int b);

int f(void) {
    return add(1, 2, 3);  // too many arguments
}
