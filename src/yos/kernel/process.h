/*
 * process.h
 *
 * the process object.
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-11   tstih
 *
 */
#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdint.h>
#include <string.h>

#include <kernel/sysobj.h>
#include <kernel/thread.h>

/* max. process name length including trailing 0 */
#define MAX_PNAME_LEN 8

/* process flags */
#define PROCESS_INTERNAL  0x01

/* process loader status */
#define PROCESS_LOAD_OK                 0
#define PROCESS_LOAD_ERR_NOT_FOUND      1
#define PROCESS_LOAD_ERR_ALLOC          2
#define PROCESS_LOAD_ERR_READ           3
#define PROCESS_LOAD_ERR_XL_INVALID     4
#define PROCESS_LOAD_ERR_XL_START       5

typedef struct process_s {
    /* process is a sys. object */
	sysobj_t hdr;
    /* process flags */
    uint8_t pflags;
    /* process name */
    char pname[MAX_PNAME_LEN];
	/* process main thread */
	thread_t *main_thread;
} process_t;

/* first process (for sysobj tracking) */
extern process_t *process_first;
extern uint8_t process_last_error;

/* load process (from file) and relocate it 
   if required. */
extern process_t *process_start(
    char *pname,
    void (*entry_point)(void),
    size_t stack_size
);

/* load process image from microdrive, relocate it, and start it */
extern process_t *process_load(
    uint8_t drive,
    char *fname,
    size_t stack_size
);

extern const char *process_last_error_text(void);

/* release resources owned by owner (events/timers/services/user heap) */
extern void process_release_owner_resources(void *owner);

/* destroy process when it has no threads left */
extern void process_cleanup_if_empty(process_t *p);

/* reap a terminated process and release all owned resources */
extern void process_reap(process_t *p);

/* detach a finished process from scheduler/process lists without freeing it */
extern void process_detach(process_t *p);

/* exit function */
extern void process_exit(void);

#endif /* __PROCESS_H__ */
