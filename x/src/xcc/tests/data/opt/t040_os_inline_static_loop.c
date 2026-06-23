static int dec_to_zero(int x) {
    while (x)
        x = x - 1;
    return x;
}

int f(int x) {
    return dec_to_zero(x);
}
