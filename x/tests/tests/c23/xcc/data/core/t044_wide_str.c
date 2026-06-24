/* t044: wide/unicode string literals — u"" and U"" are accepted */

int main(void) {
    /* Just verify they compile and have the expected first character */
    const unsigned short *u = u"hello";
    const unsigned int   *U = U"world";
    if (u[0] != 'h') return 0;
    if (U[0] != 'w') return 0;
    return 1;
}
