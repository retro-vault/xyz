// Tests [[sdcc::sdccall(1)]] caller: a/b in HL/DE, c on stack, caller pops it.
[[sdcc::sdccall(1)]] int add3(int a, int b, int c);

int main(void) {
    return add3(1, 2, 3);
}
