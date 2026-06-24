static int helper(int x) {
    if (x)
        return x + 1;
    return x + 2;
}

int use_branch_helper(int x) {
    return helper(x) ^ 3;
}
