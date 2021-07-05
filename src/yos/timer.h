/*
 * timer.h
 *
 * timer management for xyz
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-23   tstih
 *
 */
#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include <stddef.h>
#include <sysobj.h>

#define EVERYTIME 0

/*
 * timer ticks 50 times per second
 */
typedef struct timer_s
{
    sysobj_t hdr;                       /* timer is a sysobj */
    void (*hook)();                     /* hook routine */
    uint16_t ticks;                     /* trigger after ticks */
    uint16_t _tick_count;               /* count ticks (internal) */
} timer_t;

/* first timer in a linked list */
extern timer_t *_tmr_first;

/* install timer */
extern timer_t *tmr_install(void (*hook)(), uint16_t ticks, void *owner);

/* uninstall timer */
extern timer_t *tmr_uninstall(timer_t *t);

/* call this function inside your interrupt to 
   execute all times that are due */
extern void _tmr_chain();

#endif /* __TIMER_H__ */