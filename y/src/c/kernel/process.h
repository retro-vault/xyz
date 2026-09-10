/*
 * Declares the YOS process object and process-loader entry points.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdint.h>
#include <string.h>

#include <kernel/sysobj.h>
#include <kernel/thread.h>

/*
 * Maximum process-name length including the terminating NUL.
 */
#define MAX_PNAME_LEN 8

/*
 * Flag used for kernel-owned or internal processes.
 */
#define PROCESS_INTERNAL  0x01

/*
 * Process load completed successfully.
 */
#define PROCESS_LOAD_OK                 0
/*
 * Process image could not be found.
 */
#define PROCESS_LOAD_ERR_NOT_FOUND      1
/*
 * Process creation failed because memory allocation failed.
 */
#define PROCESS_LOAD_ERR_ALLOC          2
/*
 * Process image could not be read.
 */
#define PROCESS_LOAD_ERR_READ           3
/*
 * Process image failed validation.
 */
#define PROCESS_LOAD_ERR_XL_INVALID     4
/*
 * Process image did not provide a usable start entry.
 */
#define PROCESS_LOAD_ERR_XL_START       5

/*
 * YOS process object.
 */
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

/*
 * Head of the global process list.
 */
extern process_t *process_first;
/*
 * Last process-loader error code.
 */
extern uint8_t process_last_error;

/*
 * Start a process from an already resolved entry point.
 */
extern process_t *process_start(
    char *pname,
    void (*entry_point)(void),
    size_t stack_size
);

/*
 * Load a process image from Microdrive, relocate it, and start it.
 */
extern process_t *process_load(
    uint8_t drive,
    char *fname,
    size_t stack_size
);

/*
 * Release a terminated process and all of its owned resources.
 */
extern void process_reap(process_t *p);

/*
 * Detach a finished process from scheduler lists without freeing it.
 */
extern void process_detach(process_t *p);

/*
 * Exit the current process.
 */
extern void process_exit(void);

#endif /* __PROCESS_H__ */
