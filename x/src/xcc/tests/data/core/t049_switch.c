// Test: switch/case/default dispatch
int day_type(int d) {
    switch (d) {
        case 0: return 2;
        case 6: return 2;
        default: return 1;
    }
}

int classify(int x) {
    switch (x) {
        case 1: return 10;
        case 2: return 20;
        case 3: return 30;
    }
    return 0;
}
