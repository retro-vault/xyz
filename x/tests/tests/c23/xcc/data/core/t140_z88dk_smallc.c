[[z88dk::smallc]] int z88_smallc_mix(char a, int b, char c);

[[z88dk::smallc]] int z88_smallc_mix(char a, int b, char c) {
    return a + b + c;
}

int main(void) {
    return z88_smallc_mix(1, 0x0203, 4);
}
