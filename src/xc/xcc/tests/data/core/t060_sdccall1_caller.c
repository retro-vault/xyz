// Tests [[sdcc::sdccall(1)]] caller: args loaded into HL/DE/BC, no stack cleanup.
[[sdcc::sdccall(1)]] int add3(int a, int b, int c);

int main(void) {
    return add3(1, 2, 3);
}
