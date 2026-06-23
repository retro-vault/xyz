// Tests [[sdcc::interrupt]]: ISR prologue saves all regs, epilogue uses reti.
[[sdcc::interrupt]] void my_isr(void) {
    int x;
    x = 1;
}
