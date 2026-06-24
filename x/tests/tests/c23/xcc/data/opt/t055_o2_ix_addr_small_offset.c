extern void sink_int_ptr(int *);

void f(int a) {
    sink_int_ptr(&a);
}
