/* t029: typedef — scalar, struct, pointer */

typedef int Int;
typedef unsigned char Byte;

typedef struct {
    Int  x;
    Int  y;
} Vec2;

typedef Vec2 *Vec2Ptr;

Int add(Int a, Int b) {
    return a + b;
}

int main(void) {
    Int  a = 10;
    Byte b = 200;

    if (add(a, 3) != 13) return 0;
    if (b != 200)        return 0;

    Vec2 v;
    v.x = 4;
    v.y = 7;
    if (v.x + v.y != 11) return 0;

    Vec2Ptr vp = &v;
    vp->x = 100;
    if (v.x != 100) return 0;

    return 1;
}
