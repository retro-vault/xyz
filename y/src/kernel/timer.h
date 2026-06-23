/*
 * Declares the YOS timer object and timer-chain helpers.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include <stddef.h>

#include <kernel/sysobj.h>

/*
 * Special timer period that fires on every scheduler tick.
 */
#define EVERYTIME 0

/*
 * Installed timer callback object.
 */
typedef struct timer_s
{
    sysobj_t hdr;                       /* timer is a sysobj */
    void (*hook)(void);                 /* hook routine */
    uint16_t ticks;                     /* trigger after ticks */
    uint16_t _tick_count;               /* count ticks (internal) */
} timer_t;

/*
 * Head of the installed timer list.
 */
extern timer_t *_tmr_first;

/*
 * Install one timer callback owned by the supplied resource owner.
 */
extern timer_t *tmr_install(void (*hook)(void), uint16_t ticks, void *owner);

/*
 * Remove one installed timer callback.
 */
extern timer_t *tmr_uninstall(timer_t *t);

/*
 * Execute all timer callbacks that are due on the current tick.
 */
extern void _tmr_chain(void);

#endif /* __TIMER_H__ */
