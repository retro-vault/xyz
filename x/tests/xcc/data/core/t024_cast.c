/* t024: explicit casts */
int main(void) {
    int x = 1000;
    char c = (char)x;    /* truncates to 8 bits */
    int y = (int)c;
    /* 1000 = 0x03E8; char = 0xE8 = -24 signed; int = -24 */
    return y == -24;
}
