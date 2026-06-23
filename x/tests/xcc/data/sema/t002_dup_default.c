// Tests sema: duplicate default label in switch is an error.
int f(int x) {
    switch (x) {
    case 1:  return 10;
    default: return 0;
    default: return -1;  // duplicate default
    }
}
