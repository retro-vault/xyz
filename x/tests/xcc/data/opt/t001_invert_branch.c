// Tests rule_invert_branch_skip: if (cond) body
// Unoptimized: jp z,then; jp end; then: ...; end:
// Optimized:   jp nz,end; then: ...; end:
int f(int x) {
    if (x) x = 1;
    return x;
}
