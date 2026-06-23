// Tests [[deprecated]]: compiler must warn at call site.
[[deprecated("use new_api instead")]] void old_api(void);

void caller(void) {
    old_api();   // must trigger deprecated warning
}
