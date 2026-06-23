/* t045: hex float literals */

int main(void) {
    /* 0x1.8p+1 = 1.5 * 2 = 3.0 */
    float f = 0x1.8p+1;
    /* convert to int to compare */
    int i = (int)f;
    if (i != 3) return 0;
    /* 0x1p0 = 1.0 */
    float g = 0x1p0;
    int j = (int)g;
    if (j != 1) return 0;
    return 1;
}
