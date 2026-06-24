// Tests sema: duplicate case value in switch is an error.
int f(int x) {
    switch (x) {
    case 1: return 10;
    case 1: return 20;   // duplicate
    default: return 0;
    }
}
