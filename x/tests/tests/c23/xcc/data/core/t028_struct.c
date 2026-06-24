/* t028: struct declaration, field read/write, pointer-to-struct */

struct Point {
    int x;
    int y;
};

struct Rect {
    struct Point tl;
    struct Point br;
};

int area(struct Rect *r) {
    int w = r->br.x - r->tl.x;
    int h = r->br.y - r->tl.y;
    return w * h;
}

int main(void) {
    struct Point p;
    p.x = 3;
    p.y = 7;
    if (p.x != 3) return 0;
    if (p.y != 7) return 0;

    /* compound assignment on struct field */
    p.x += 2;
    if (p.x != 5) return 0;

    /* pointer to struct */
    struct Point *pp = &p;
    pp->y = 10;
    if (p.y != 10) return 0;

    /* nested struct / area() */
    struct Rect r;
    r.tl.x = 1; r.tl.y = 2;
    r.br.x = 4; r.br.y = 6;
    if (area(&r) != 12) return 0;

    return 1;
}
