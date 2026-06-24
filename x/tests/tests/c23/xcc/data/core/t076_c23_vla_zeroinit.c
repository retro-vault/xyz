// C23: int vla[n] = {} zero-initializes the VLA via __vla_zero().
void zero_test(int n) {
    int buf[n];
    int zeros[n];
    (void)buf;
    (void)zeros;
}
