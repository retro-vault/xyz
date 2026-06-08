[[sdcc::sdccall(1)]] extern int foreign_add(int a, int b);

int call_foreign_override(int a, int b) {
    return foreign_add(a, b);
}
