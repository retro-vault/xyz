// Tests jp_next removal + invert_branch in loop constructs
int sum(int n) {
    int s = 0;
    while (n > 0) { s += n; n--; }
    return s;
}
