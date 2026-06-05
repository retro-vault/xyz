/*
 * signal.c
 *
 * Minimal process-wide signal disposition support for the xcc Z80 libc.
 *
 * This freestanding target keeps one small table of ISO C signal handlers.
 * raise() runs the installed disposition synchronously in the current thread
 * of execution. Default actions fall back to the libc abort sink.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <signal.h>
#include <stdlib.h>

static __signal_handler_t __signal_handlers[7] = {
    SIG_DFL,
    SIG_DFL,
    SIG_DFL,
    SIG_DFL,
    SIG_DFL,
    SIG_DFL,
    SIG_DFL
};

static int __signal_valid(int sig)
{
    return sig >= SIGABRT && sig <= SIGTERM;
}

__signal_handler_t signal(int sig, __signal_handler_t func)
{
    __signal_handler_t old;

    if (!__signal_valid(sig)) {
        return SIG_ERR;
    }

    old = __signal_handlers[sig];
    __signal_handlers[sig] = func;
    return old;
}

int raise(int sig)
{
    __signal_handler_t handler;

    if (!__signal_valid(sig)) {
        return 1;
    }

    handler = __signal_handlers[sig];
    if (handler == SIG_IGN) {
        return 0;
    }
    if (handler == SIG_DFL) {
        abort();
    }

    handler(sig);
    return 0;
}
