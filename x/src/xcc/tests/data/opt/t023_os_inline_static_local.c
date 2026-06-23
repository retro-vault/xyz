static int helper(int x) {
    int y = x + 1;
    return y;
}

int use_local_helper(int x) {
    return helper(x) + 3;
}
