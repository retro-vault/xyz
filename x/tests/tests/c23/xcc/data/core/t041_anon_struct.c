/* t041: anonymous struct/union — fields promoted to enclosing scope */

struct Vec3 {
    union {
        struct { int x; int y; int z; };
        int v[3];
    };
};

int main(void) {
    struct Vec3 u;
    u.x = 1;
    u.y = 2;
    u.z = 3;

    if (u.x != 1) return 0;
    if (u.y != 2) return 0;
    if (u.z != 3) return 0;

    /* access via array member of the anonymous union */
    if (u.v[0] != 1) return 0;
    if (u.v[1] != 2) return 0;
    if (u.v[2] != 3) return 0;

    return 1;
}
