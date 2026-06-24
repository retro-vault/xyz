/* t032: compound assignment on array subscript — a[i] += n */
int main(void) {
    int a[3];
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    int i = 1;
    a[i] += 10;
    a[0] -= 1;
    int ok = (a[0] == 0 && a[1] == 12 && a[2] == 3);
    return ok;
}
