/*
 * time.h
 *
 * Standard C date and time support for the xcc Z80 target.
 *
 * The calendar and formatting routines are target-independent and are built
 * entirely on top of two platform hooks supplied by the selected sys backend
 * (lib/sys/<backend>/):
 *
 *     int gettimeofday(struct timespec *tv);        // read wall clock
 *     int settimeofday(const struct timespec *tv);  // set  wall clock
 *
 * The "none" backend provides empty shells (epoch 0); an operating system
 * replaces them to wire in a real clock, after which the whole of <time.h>
 * works without further changes.
 *
 * There is no timezone or DST model: local time equals UTC.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _TIME_H
#define _TIME_H

#include <stddef.h>     /* size_t, NULL */

typedef long time_t;    /* seconds since 1970-01-01 00:00:00 UTC */
typedef long clock_t;   /* processor-time ticks                  */

#define CLOCKS_PER_SEC ((clock_t)1)
#define TIME_UTC       1

struct tm {
    int tm_sec;     /* seconds after the minute     [0, 60] */
    int tm_min;     /* minutes after the hour       [0, 59] */
    int tm_hour;    /* hours since midnight          [0, 23] */
    int tm_mday;    /* day of the month              [1, 31] */
    int tm_mon;     /* months since January          [0, 11] */
    int tm_year;    /* years since 1900                      */
    int tm_wday;    /* days since Sunday             [0, 6]  */
    int tm_yday;    /* days since January 1          [0, 365]*/
    int tm_isdst;   /* daylight saving flag (always 0 here)  */
};

struct timespec {
    time_t tv_sec;  /* whole seconds      */
    long   tv_nsec; /* nanoseconds [0, 1e9) */
};

/* ------------------------------------------------------------------------- */
/* Time manipulation                                                         */
/* ------------------------------------------------------------------------- */
clock_t clock(void);
time_t  time(time_t *timer);
double  difftime(time_t end, time_t beginning);
time_t  mktime(struct tm *timeptr);
int     timespec_get(struct timespec *ts, int base);
int     timespec_getres(struct timespec *ts, int base);

/* ------------------------------------------------------------------------- */
/* Conversion to broken-down and textual forms                               */
/* ------------------------------------------------------------------------- */
struct tm *gmtime(const time_t *timer);
struct tm *gmtime_r(const time_t *timer, struct tm *result);
struct tm *localtime(const time_t *timer);
struct tm *localtime_r(const time_t *timer, struct tm *result);

char *asctime(const struct tm *timeptr);
char *asctime_r(const struct tm *timeptr, char *buf);
char *ctime(const time_t *timer);
char *ctime_r(const time_t *timer, char *buf);

size_t strftime(char *restrict s, size_t maxsize,
                const char *restrict format,
                const struct tm *restrict timeptr);

/* ------------------------------------------------------------------------- */
/* Platform clock hooks (supplied by the sys backend)                        */
/* ------------------------------------------------------------------------- */
int gettimeofday(struct timespec *tv);
int settimeofday(const struct timespec *tv);

#endif /* _TIME_H */
