// The compatible extra-c spelling must select ABI 1 and preserve DE:HL.
[[sdcc::sdcccall(1)]] long add3l(int a, int b, int c);

int main(void) {
    return (int)add3l(1, 2, 3);
}
