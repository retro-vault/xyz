#include "xcc_exec_test.h"

#include <threads.h>

static int once_hits;
static once_flag flag = ONCE_FLAG_INIT;
static mtx_t mtx;
static cnd_t cnd;
static tss_t key;
static int join_result;
static struct timespec req;
static struct timespec rem;

static void once_body(void) {
    ++once_hits;
}

static int never_run(void *arg) {
    return arg != (void *)0;
}

int main(void) {
    join_result = 99;
    req.tv_sec = 1L;
    req.tv_nsec = 2L;
    rem.tv_sec = 9L;
    rem.tv_nsec = 9L;

    call_once(&flag, once_body);
    call_once(&flag, once_body);
    if (once_hits != 1) return 1;

    if (mtx_init(&mtx, mtx_plain) != thrd_success) return 2;
    if (mtx_trylock(&mtx) != thrd_success) return 3;
    if (mtx_trylock(&mtx) != thrd_busy) return 4;
    if (mtx_unlock(&mtx) != thrd_success) return 5;
    mtx_destroy(&mtx);

    if (mtx_init(&mtx, mtx_recursive) != thrd_success) return 6;
    if (mtx_lock(&mtx) != thrd_success) return 7;
    if (mtx_lock(&mtx) != thrd_success) return 8;
    if (mtx_unlock(&mtx) != thrd_success) return 9;
    if (mtx_unlock(&mtx) != thrd_success) return 10;
    if (mtx_timedlock(&mtx, &req) != thrd_success) return 11;
    if (mtx_timedlock(&mtx, &req) != thrd_success) return 12;
    if (mtx_unlock(&mtx) != thrd_success) return 13;
    if (mtx_unlock(&mtx) != thrd_success) return 14;

    if (cnd_init(&cnd) != thrd_success) return 15;
    if (cnd_signal(&cnd) != thrd_success) return 16;
    if (cnd_wait(&cnd, &mtx) != thrd_success) return 17;
    if (cnd_timedwait(&cnd, &mtx, &req) != thrd_timedout) return 18;
    if (cnd_broadcast(&cnd) != thrd_success) return 19;
    if (cnd_wait(&cnd, &mtx) != thrd_success) return 20;
    cnd_destroy(&cnd);

    if (thrd_current() != 1u) return 21;
    if (!thrd_equal(thrd_current(), 1u)) return 22;
    if (thrd_equal(thrd_current(), 2u)) return 23;
    thrd_yield();
    if (thrd_sleep(&req, &rem) != 0) return 24;
    if (rem.tv_sec != 0L || rem.tv_nsec != 0L) return 25;

    if (thrd_create((thrd_t *)0, never_run, (void *)0) != thrd_error) return 26;
    if (thrd_detach(1u) != thrd_error) return 27;
    if (thrd_join(1u, &join_result) != thrd_error) return 28;
    if (join_result != 0) return 29;

    if (tss_create(&key, (tss_dtor_t)0) != thrd_success) return 30;
    if (tss_set(key, (void *)0x1234u) != thrd_success) return 31;
    if (tss_get(key) != (void *)0x1234u) return 32;
    tss_delete(key);
    if (tss_get(key) != (void *)0) return 33;

    return 0;
}
