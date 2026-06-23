/* t025: shift operators */
int main(void) {
    int x = 1;
    int a = x << 3;   /* 8 */
    int b = a >> 1;   /* 4 */
    return (a == 8) && (b == 4);
}
