// Tests rule_jp_to_jr: short forward branches converted jp->jr
int f(int a, int b) {
    return a < b ? a : b;
}
