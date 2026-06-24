static int picka(int x) {
    int y = x;
    if (y)
        return y + 1;
    return 0;
}

static int pickb(int x) {
    int y = x;
    if (y)
        return y + 1;
    return 0;
}

int use_pick(int a, int b) {
    return picka(a) + pickb(b);
}
