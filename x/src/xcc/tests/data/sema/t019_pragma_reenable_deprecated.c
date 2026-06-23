[[deprecated("use newer")]] void old_api(void);
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void muted(void) { old_api(); }
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
void noisy(void) { old_api(); }
