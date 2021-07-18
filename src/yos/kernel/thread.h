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
#include <stdbool.h>

#include <kernel/list.h>
#include <kernel/sysobj.h>
#include <kernel/evt.h>
#include <kernel/mem.h>
#include <kernel/interrupts.h>
#include <kernel/timer.h>

#define THREAD_STATE_SUSPENDED      0
#define THREAD_STATE_RUNNING        1
#define THREAD_STATE_WAITING        2
#define THREAD_STATE_JOINED         3
#define THREAD_STATE_TERMINATED     4

/* context size is the size of all register + return address 
   af+bc+hl+de+ix+iy+af'+bc'+de+hl+ret addr = 22 bytes
   sp is stored to sp member of the thread_s structure */
#define CONTEXT_SIZE			    22

typedef struct thread_s {
    /* thread is a sys. object */
	sysobj_t hdr;
    uint16_t sp;                        /* stack pointer. task context is stored on stack. */
	/* thread properties */
    uint8_t startup[10];                /* startup code: CALL+JP */
	event_t **wait;                     /* event list or null */
	uint8_t num_events;                 /* number of events in event list */
	uint8_t state;                      /* thread state (bits 0-1), bits 2-7 are reserved */
    struct thread_s **joined;           /* joined threads */
    void *process;                      /* parent procsss */
} thread_t;

extern thread_t *thread_current;        /* current runnnig thread */
extern thread_t *thread_first_suspended;
extern thread_t *thread_first_running;
extern thread_t *thread_first_waiting;
extern thread_t *thread_first_terminated;

/* exit is called at end of thread */
extern void thread_exit(thread_t *t);

/* select next thread to run */
extern thread_t* _thread_select_next();

/* context switch */
extern void _thread_robin() __naked;

/* create new thread */
extern thread_t * thread_create(void (*entry_point)(), uint16_t stack_size);

/* kill (forcefully abort!) a thread */
extern void thread_kill(thread_t *t);

/* wait for sync. events */
extern void thread_wait4events(event_t **events, uint8_t num_events);

/* pause thread...*/
extern void thread_suspend(thread_t *t);

/* unpause thread */
extern void thread_resume(thread_t *t);

/* join a thread */
extern void thread_join(thread_t *t);

#endif /* __THREAD_H__ */