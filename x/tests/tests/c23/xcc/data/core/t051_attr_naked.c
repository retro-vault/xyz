// Tests [[sdcc::naked]]: no prologue or epilogue emitted.
[[sdcc::naked]] void isr_stub(void) {
    __asm__("reti");
}
