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
#define MAX_PNAME_LEN 16

/* process flags */
#define PROCESS_INTERNAL  0x01

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

/* load process (from file) and relocate it 
   if required. */
extern process_t *process_start(
    char *pname,
    void *entry_point,
    size_t stack_size
);

/* exit function */
extern void process_exit();

#endif /* __PROCESS_H__ */