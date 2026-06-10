/*
 * threads.h
 *
 * Single-threaded C11 threads API for the xcc Z80 target.
 *
 * This target currently exposes a deterministic freestanding fallback rather
 * than a preemptive scheduler:
 *   - once flags, mutexes, and thread-specific storage are implemented
 *   - condition variables are local signaling shims
 *   - true thread creation/join/detach are reported as unsupported
 *
 * The interface is complete enough for portable code to compile and for
 * single-threaded libraries to use the synchronization primitives without
 * pulling in a hosted OS.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _THREADS_H
#define _THREADS_H

#include <time.h>

typedef unsigned short thrd_t;
typedef int (*thrd_start_t)(void *);
typedef void (*tss_dtor_t)(void *);

typedef unsigned char once_flag;

typedef struct {
    unsigned char _state;
    unsigned char _flags;
    unsigned short _count;
} mtx_t;

typedef struct {
    unsigned char _state;
} cnd_t;

typedef unsigned char tss_t;

#define ONCE_FLAG_INIT      ((once_flag)0)
#define TSS_DTOR_ITERATIONS 1

enum {
    thrd_success  = 0,
    thrd_busy     = 1,
    thrd_error    = 2,
    thrd_nomem    = 3,
    thrd_timedout = 4
};

enum {
    mtx_plain     = 0,
    mtx_recursive = 1,
    mtx_timed     = 2
};

void call_once(once_flag *flag, void (*func)(void));

int  cnd_broadcast(cnd_t *cond);
void cnd_destroy(cnd_t *cond);
int  cnd_init(cnd_t *cond);
int  cnd_signal(cnd_t *cond);
int  cnd_timedwait(cnd_t *restrict cond,
                   mtx_t *restrict mtx,
                   const struct timespec *restrict ts);
int  cnd_wait(cnd_t *cond, mtx_t *mtx);

void mtx_destroy(mtx_t *mtx);
int  mtx_init(mtx_t *mtx, int type);
int  mtx_lock(mtx_t *mtx);
int  mtx_timedlock(mtx_t *restrict mtx,
                   const struct timespec *restrict ts);
int  mtx_trylock(mtx_t *mtx);
int  mtx_unlock(mtx_t *mtx);

int    thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
thrd_t thrd_current(void);
int    thrd_detach(thrd_t thr);
int    thrd_equal(thrd_t lhs, thrd_t rhs);
_Noreturn void thrd_exit(int res);
int    thrd_join(thrd_t thr, int *res);
int    thrd_sleep(const struct timespec *duration,
                  struct timespec *remaining);
void   thrd_yield(void);

int   tss_create(tss_t *key, tss_dtor_t dtor);
void  tss_delete(tss_t key);
void *tss_get(tss_t key);
int   tss_set(tss_t key, void *val);

#endif /* _THREADS_H */
