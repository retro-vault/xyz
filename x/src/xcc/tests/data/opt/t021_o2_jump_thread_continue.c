static int skip_one(void) {
    int sum = 0;
    int i = 0;

    while (i < 4) {
        if (i == 1) {
            ++i;
            continue;
        }
        sum += i;
        ++i;
    }

    return sum;
}

int f(void) {
    return skip_one();
}
