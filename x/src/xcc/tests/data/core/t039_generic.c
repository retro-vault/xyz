/* t039: _Generic selection — inline (no macro, xcc handles preprocessing externally) */

int int_val(int x)   { return x + 1; }
int char_val(char x) { return x + 2; }

int main(void) {
    int  i = 10;
    char c = 10;

    /* int branch: 10+1=11 */
    if (_Generic(i, int: int_val(i), char: char_val(i), default: 0) != 11) return 0;

    /* char branch: 10+2=12 */
    if (_Generic(c, int: int_val(c), char: char_val(c), default: 0) != 12) return 0;

    /* default branch: short has no match */
    short s = 5;
    if (_Generic(s, int: 1, char: 2, default: 99) != 99) return 0;

    return 1;
}
