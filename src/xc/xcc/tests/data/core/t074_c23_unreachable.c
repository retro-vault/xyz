// C23: __builtin_unreachable() — marks dead code paths.
[[noreturn]] void panic(void) {
    __builtin_unreachable();
}

int safe_div(int a, int b) {
    if (b == 0) { panic(); __builtin_unreachable(); }
    return a / b;
}
