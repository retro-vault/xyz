/* t027: _Static_assert, _Alignof, __func__, aggregate init, wide string prefix */

/* _Static_assert at file scope */
_Static_assert(sizeof(int) == 2, "int must be 2 bytes on Z80");
_Static_assert(sizeof(char) == 1, "char must be 1 byte");
_Static_assert(1 + 1 == 2, "math works");

/* _Alignof */
int test_alignof(void) {
    int a = _Alignof(int);
    int b = _Alignof(char);
    return (a == 2 && b == 1) ? 1 : 0;
}

/* __func__ — returns pointer to function name string; just check it's non-null */
int test_func_name(void) {
    const char *name = __func__;
    return (name != 0) ? 1 : 0;
}

/* aggregate array initializer */
int test_array_init(void) {
    int a[4] = {10, 20, 30, 40};
    return (a[0] == 10 && a[1] == 20 && a[2] == 30 && a[3] == 40) ? 1 : 0;
}

/* Wide/unicode string prefix stripped — treated as plain string */
int test_wide_string(void) {
    const char *s = L"hello";
    return (s != 0) ? 1 : 0;
}

int main(void) {
    if (!test_alignof())    return 0;
    if (!test_func_name())  return 0;
    if (!test_array_init()) return 0;
    if (!test_wide_string()) return 0;
    return 1;
}
