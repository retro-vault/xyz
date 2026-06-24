#include <stdarg.h>

#include "log.h"

static const char *level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static log_LockFn lock_fn;
static void *lock_udata;
static log_LogFn callback_fn;
static void *callback_udata;
static int callback_level;
static int quiet;
static int current_level;

const char *log_level_string(int level) {
    return level_strings[level];
}

void log_set_lock(log_LockFn fn, void *udata) {
    lock_fn = fn;
    lock_udata = udata;
}

void log_set_level(int level) {
    current_level = level;
}

void log_set_quiet(bool enable) {
    quiet = enable;
}

int log_add_callback(log_LogFn fn, void *udata, int level) {
    callback_fn = fn;
    callback_udata = udata;
    callback_level = level;
    return 0;
}

int log_add_fp(FILE *fp, int level) {
    (void)fp;
    (void)level;
    return 0;
}

void log_log(int level, const char *file, int line, const char *fmt, ...) {
    log_Event ev;
    va_list ap;
    (void)quiet;
    (void)current_level;

    if (lock_fn) lock_fn(true, lock_udata);
    if (callback_fn && level >= callback_level) {
        va_start(ap, fmt);
        ev.ap = ap;
        ev.fmt = fmt;
        ev.file = file;
        ev.time = 0;
        ev.udata = callback_udata;
        ev.line = line;
        ev.level = level;
        callback_fn(&ev);
        va_end(ap);
    }
    if (lock_fn) lock_fn(false, lock_udata);
}
