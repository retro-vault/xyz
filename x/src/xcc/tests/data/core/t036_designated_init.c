/* t036: designated initializers for structs and arrays */

struct Point { int x; int y; };
struct RGB   { int r; int g; int b; };

int test_struct_designated(void) {
    struct Point p = { .y = 10, .x = 5 };
    return (p.x == 5 && p.y == 10) ? 1 : 0;
}

int test_array_designated(void) {
    int a[4] = { [2] = 30, [0] = 10 };
    return (a[0] == 10 && a[2] == 30) ? 1 : 0;
}

int test_mixed(void) {
    struct RGB c = { .r = 255, .b = 128 };
    return (c.r == 255 && c.b == 128) ? 1 : 0;
}

int main(void) {
    if (!test_struct_designated()) return 0;
    if (!test_array_designated())  return 0;
    if (!test_mixed())             return 0;
    return 1;
}
