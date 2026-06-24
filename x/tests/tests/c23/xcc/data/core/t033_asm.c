/* t033: __asm__ inline assembly — nop should not affect result */
int main(void) {
    int x = 42;
    __asm__("nop");
    return x;
}
