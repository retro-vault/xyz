// A void sdcccall(1) callee removes its own stack-spilled arguments.
extern void plot(void *glyph, int x, int y);

int main(void) {
    plot((void *)0x1234, 140, 180);
    return 0;
}
