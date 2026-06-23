// Tests [[noreturn]]: standard C23 attribute, compiles cleanly.
[[noreturn]] void halt(void);

int main(void) {
    halt();
    return 0;
}
