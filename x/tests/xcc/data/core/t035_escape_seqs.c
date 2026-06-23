/* t035: escape sequences — hex and octal */
int main(void) {
    char a = '\x41';    /* 'A' = 65 */
    char b = '\101';    /* octal 101 = 'A' = 65 */
    int ok = 1;
    if (a != 65) ok = 0;
    if (b != 65) ok = 0;
    return ok;
}
