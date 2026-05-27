/* t038: compound literals — (Type){initializer} */

struct Point {
    int x;
    int y;
};

int sum(struct Point p) {
    return p.x + p.y;
}

int main(void) {
    /* compound literal passed to function */
    if (sum((struct Point){3, 4}) != 7) return 0;

    /* compound literal assigned to local */
    struct Point p = (struct Point){10, 20};
    if (p.x != 10) return 0;
    if (p.y != 20) return 0;

    /* compound literal with designated init */
    struct Point q = (struct Point){.y = 5, .x = 2};
    if (q.x != 2) return 0;
    if (q.y != 5) return 0;

    return 1;
}
