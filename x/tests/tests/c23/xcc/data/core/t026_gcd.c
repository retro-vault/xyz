/* t026: Euclidean GCD — integration test */
int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a - (a / b) * b;
        a = t;
    }
    return a;
}

int main(void) {
    return gcd(48, 18);
}
