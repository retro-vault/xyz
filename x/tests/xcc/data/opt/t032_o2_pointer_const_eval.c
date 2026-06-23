static void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int f(void) {
    int x = 10;
    int y = 20;
    int z = 5;
    int *p = &z;

    swap(&x, &y);
    *p = 42;
    return x + y + z;
}
