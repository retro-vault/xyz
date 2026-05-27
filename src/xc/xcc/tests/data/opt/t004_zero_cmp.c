// Tests rule_zero_cmp_optimize: comparison with zero uses or a,a / cp 0 elimination
int f(int x) {
    return x != 0;
}
