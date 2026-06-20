#include <string.h>

#include "log.h"

static void lock_fn(bool lock, void *udata) {
    (void)lock;
    (void)udata;
}

int main(void) {
    if (strcmp(log_level_string(LOG_WARN), "WARN") != 0) return 1;
    log_set_lock(lock_fn, 0);
    log_set_quiet(true);
    log_set_level(LOG_INFO);
    return 0;
}
