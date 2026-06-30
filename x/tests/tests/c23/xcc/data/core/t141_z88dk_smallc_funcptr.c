[[z88dk::smallc]] int z88_smallc_add2(int a, int b) {
    return a + b;
}

int main(void) {
    [[z88dk::smallc]] int (*fp)(int, int) = z88_smallc_add2;
    return fp(5, 6);
}
