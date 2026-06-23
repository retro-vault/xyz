/* t016: nested if-else */
int classify(int n) {
    if (n < 0) {
        return -1;
    } else if (n == 0) {
        return 0;
    } else {
        return 1;
    }
}

int main(void) {
    int a = classify(-5);
    int b = classify(0);
    int c = classify(42);
    return (a == -1) && (b == 0) && (c == 1);
}
