// Tests [[sdcc::critical]]: prologue emits di, epilogue emits ei before ret.
[[sdcc::critical]] void atomic_op(void) {
    int x;
    x = 42;
}
