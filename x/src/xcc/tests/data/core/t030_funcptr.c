/* t030: function pointers — declaration, assignment, call, typedef */

typedef int (*BinOp)(int, int);

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }

/* apply op to a and b */
int apply(BinOp op, int a, int b) {
    return op(a, b);
}

/* select op based on code */
BinOp pick(int code) {
    if (code == 0) return add;
    if (code == 1) return sub;
    return mul;
}

int main(void) {
    /* direct function pointer variable */
    int (*fp)(int, int) = add;
    if (fp(3, 4) != 7) return 0;

    fp = sub;
    if (fp(10, 3) != 7) return 0;

    /* pass function pointer to another function */
    if (apply(mul, 3, 4) != 12) return 0;
    if (apply(add, 5, 6) != 11) return 0;

    /* return function pointer from function */
    BinOp op = pick(0);
    if (op(2, 3) != 5) return 0;

    op = pick(1);
    if (op(9, 4) != 5) return 0;

    op = pick(2);
    if (op(3, 4) != 12) return 0;

    return 1;
}
