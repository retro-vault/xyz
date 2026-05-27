/* t040: bit-fields — pack, read, write */

struct Flags {
    unsigned int a : 3;   /* bits 0-2 */
    unsigned int b : 4;   /* bits 3-6 */
    unsigned int c : 1;   /* bit  7   */
};

int main(void) {
    struct Flags f;
    f.a = 5;   /* 0b101 */
    f.b = 9;   /* 0b1001 */
    f.c = 1;

    if (f.a != 5) return 0;
    if (f.b != 9) return 0;
    if (f.c != 1) return 0;

    f.a = 3;
    if (f.a != 3) return 0;
    /* b and c must be unchanged */
    if (f.b != 9) return 0;
    if (f.c != 1) return 0;

    return 1;
}
