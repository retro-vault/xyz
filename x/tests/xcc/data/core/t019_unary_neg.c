/* t019: unary minus and logical not */
int main(void) {
    int x = 5;
    int y = -x;
    int z = !x;
    return (y == -5) && (z == 0);
}
