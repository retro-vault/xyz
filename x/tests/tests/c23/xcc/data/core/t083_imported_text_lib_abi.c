extern int foreign_add(int a, int b);

int call_foreign_lib(int a, int b) {
    return foreign_add(a, b);
}
