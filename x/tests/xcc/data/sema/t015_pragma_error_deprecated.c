[[deprecated("use newer")]] void old_api(void);

#pragma GCC diagnostic error "-Wdeprecated-declarations"

void caller(void) {
    old_api();
}
