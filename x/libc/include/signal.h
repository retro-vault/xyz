/*
 * signal.h
 *
 * Standard C23 signal registration support for the xcc Z80 target.
 *
 * This libc keeps one process-wide disposition table for the core ISO C
 * signals. raise() invokes the registered handler directly in the current
 * thread of execution, and default actions terminate the program with an
 * internal abort loop.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _SIGNAL_H
#define _SIGNAL_H

#define __STDC_VERSION_SIGNAL_H__ 202311L

typedef int sig_atomic_t;
typedef void (*__signal_handler_t)(int);

#define SIGABRT 1
#define SIGFPE  2
#define SIGILL  3
#define SIGINT  4
#define SIGSEGV 5
#define SIGTERM 6

#define SIG_DFL ((__signal_handler_t)0)
#define SIG_IGN ((__signal_handler_t)1)
#define SIG_ERR ((__signal_handler_t)-1)

__signal_handler_t signal(int sig, __signal_handler_t func);
int raise(int sig);

#endif /* _SIGNAL_H */
