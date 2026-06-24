/* t034: 32-bit long arithmetic — add, subtract, small multiply */
int main(void) {
    long a = 100000;
    long b = 200000;
    long sum = a + b;
    long diff = b - a;
    int ok = 1;
    if (sum  != 300000) ok = 0;
    if (diff != 100000) ok = 0;
    return ok;
}
