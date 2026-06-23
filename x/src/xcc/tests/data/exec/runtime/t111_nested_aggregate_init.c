struct option {
    int type;
    const char short_name;
    const char *long_name;
    void *value;
    int flags;
};

int main(void) {
    int force = 0;
    int number = 0;
    struct option options[3] = {
        { 2, 'f', "force", &force, 0 },
        { 4, 'n', "number", &number, 1 },
        { 0, 0, 0, 0, 0 }
    };

    if (options[0].type != 2) return 1;
    if (options[0].short_name != 'f') return 2;
    if (options[0].long_name[0] != 'f') return 3;
    if (options[0].value != &force) return 4;
    if (options[1].type != 4) return 5;
    if (options[1].short_name != 'n') return 6;
    if (options[1].long_name[0] != 'n') return 7;
    if (options[1].value != &number) return 8;
    if (options[1].flags != 1) return 9;
    if (options[2].type != 0 || options[2].value != 0) return 10;
    return 0;
}
