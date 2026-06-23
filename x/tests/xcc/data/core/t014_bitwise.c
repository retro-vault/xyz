/* t014: bitwise operations */
int main(void) {
    int a = 0x0F;
    int b = 0xFF;
    int c = a & b;    /* 0x0F */
    int d = a | 0xF0; /* 0xFF */
    int e = a ^ 0xFF; /* 0xF0 */
    return (c == 0x0F) && (d == 0xFF) && (e == 0xF0);
}
