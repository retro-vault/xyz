// C23: auto type deduction — type inferred from initializer.
int main(void) {
    auto x = 42;
    auto p = &x;
    *p = 10;
    return x;
}
