/* t043: variable-length arrays */

int sum_vla(int n) {
    int a[n];
    int i;
    for (i = 0; i < n; i = i + 1)
        a[i] = i + 1;
    int total = 0;
    for (i = 0; i < n; i = i + 1)
        total = total + a[i];
    return total;
}

int main(void) {
    /* sum 1..4 = 10 */
    if (sum_vla(4) != 10) return 0;
    /* sum 1..1 = 1 */
    if (sum_vla(1) != 1) return 0;
    return 1;
}
