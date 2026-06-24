extern int foreign_add(int a, int b);

int call_foreign(int a, int b) {
    return foreign_add(a, b);
}
