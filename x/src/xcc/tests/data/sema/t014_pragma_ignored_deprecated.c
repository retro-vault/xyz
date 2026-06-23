[[deprecated("use newer")]] void old_api(void);

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

void caller(void) {
    old_api();
}
