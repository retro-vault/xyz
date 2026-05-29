// C23: storage class specifier in compound literals (static keyword).
int *get_table(void) {
    return (static int[]){1, 2, 3, 4};
}

const char *get_msg(void) {
    return (static const char[]){"hello"};
}
