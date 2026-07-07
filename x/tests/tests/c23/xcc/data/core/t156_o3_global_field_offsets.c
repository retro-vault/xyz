struct api {
    int (*first)(void);
    int (*second)(void);
};

struct api table;

int one(void) {
    return 1;
}

int two(void) {
    return 2;
}

void init(void) {
    table.first = one;
    table.second = two;
}
