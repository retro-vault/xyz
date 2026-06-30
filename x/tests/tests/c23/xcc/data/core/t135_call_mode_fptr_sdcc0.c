// Tests --sdcccall 0 for an unannotated function-pointer call.
typedef int (*Op)(int, int);

int call_op(Op op, int a, int b) {
    return op(a, b);
}
