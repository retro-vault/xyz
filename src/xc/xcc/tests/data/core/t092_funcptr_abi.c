typedef int (*StackOp)(int, int) [[sdcc::sdccall(0)]];
typedef int (*RegOp)(int, int);

int call_stack(StackOp op, int a, int b) {
    return op(a, b);
}

int call_reg(RegOp op, int a, int b) {
    return op(a, b);
}
