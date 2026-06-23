/* t022: break and continue */
int main(void) {
    int sum = 0;
    int i = 0;
    while (1) {
        if (i >= 5) break;
        i = i + 1;
        if (i == 3) continue;
        sum = sum + i;
    }
    /* sum = 1+2+4+5 = 12 */
    return sum;
}
