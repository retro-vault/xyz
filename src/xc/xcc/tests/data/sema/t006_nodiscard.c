// Tests [[nodiscard]]: compiler must warn when return value is discarded.
[[nodiscard]] int important(void);

void caller(void) {
    important();   // return value discarded — must warn
}
