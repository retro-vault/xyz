// Tests sema: existing const-assignment check still fires.
void f(void) {
    const int x = 5;
    x = 10;  // error: const lvalue
}
