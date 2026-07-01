// Tests default sdcccall(1): wider-than-16-bit returns keep stack args caller-clean.
[[sdcc::sdcccall(1)]] long add3l(int a, int b, int c);

int main(void) {
    return (int)add3l(1, 2, 3);
}
