/*
 * thread.h
 *
 * the thread object.
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-23   tstih
 *
 */
#ifndef __THREAD_H__
#define __THREAD_H__

#include <stdint.h>

#include <list.h>
#include <sysobj.h>
#include <evt.h>

#define THREAD_STATE_SUSPENDED      0
#define THREAD_STATE_RUNNING        1
#define THREAD_STATE_WAITING        2
#define THREAD_STATE_TERMINATED     3

#define CONTEXT_SIZE			22

typedef struct thread_s {
    /* thread is a sys. object */
	sysobj_t hdr;
	/* thread properties */
	uint16_t sp;                        /* stack pointer. task context is stored on stack. */
	event_t **wait;                     /* event list or null */
	uint8_t num_events;                 /* number of events in event list */
	uint8_t state;                      /* thread state (bits 0-1), bits 2-7 are reserved */
} thread_t;

extern thread_t *tsk_current;
extern thread_t *tsk_first_running;
extern thread_t *tsk_first_waiting;

extern thread_t * thread_create(void (*entry_point)(), uint16_t stack_size);
extern void thread_wait4events(event_t **events, uint8_t num_events);
extern void thread_switch();
extern void thread_suspend(thread_t t);
extern void thread_resume(thread_t t);
extern void thread_kill(thread_t t);

#endif /* __THREAD_H__ */