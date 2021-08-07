/*
 * process.c
 *
 * the process functions.
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-15   tstih
 *
 */
#include <kernel/process.h>

process_t *process_first;

process_t *process_start(
    char *pname,
    void (*entry_point)(),
    size_t stack_size
) {
    /* first create new process */
    process_t *p;
	if ( p = (process_t *)so_create(
            (void **)&process_first, sizeof(process_t), NONE
        ) 
    ) {
        /* populate the process */
		strcpy(p->pname,pname);
        p->pflags=0;
        /* create main thread and...*/
        p->main_thread=thread_create(
            entry_point,
            stack_size, 
            (void *)p);
        p->main_thread->process=p;
        /* ...start it! */
        thread_resume(p->main_thread);
	}
	return p;
}

void _process_cleanup(process_t *p) {
    p;
}

void process_exit() {
    /* get current process */
    process_t *proc=thread_current->process;
    /* clean up all resources */
    _process_cleanup(proc);
    /* finally, remove from process list */
    so_destroy((void **)&process_first, (void *)proc);
}