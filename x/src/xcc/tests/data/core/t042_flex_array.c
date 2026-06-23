/* t042: flexible array member — struct S { int n; int data[]; }; */

struct Buf {
    int  len;
    char data[];
};

int main(void) {
    /* sizeof excludes the flexible member */
    if (sizeof(struct Buf) != 2) return 0;

    /* a static buffer with tail data (compound init) */
    struct { int len; char data[4]; } b;
    b.len = 4;
    b.data[0] = 'a';
    b.data[1] = 'b';
    b.data[2] = 'c';
    b.data[3] = '\0';
    if (b.len != 4) return 0;
    if (b.data[0] != 'a') return 0;

    return 1;
}
