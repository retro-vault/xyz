// C23: [[noreturn]] suppresses epilogue in the convention output.
[[noreturn]] void halt_forever(void) {
    while (1) {}
}

[[noreturn]] [[sdcc::naked]] void raw_halt(void) {
    __asm__("halt");
    __asm__("jp 0");
}
