/*
 * thread.c
 *
 * threading logic
 * TODO: guard ops with critical-section helpers
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-23   tstih
 *
 */
#include <kernel/thread.h>
#include <kernel/process.h>
#include <kernel/list.h>

thread_t *thread_current = NULL;
thread_t *thread_first_suspended = NULL;
thread_t *thread_first_running = NULL;
thread_t *thread_first_waiting = NULL;
thread_t *thread_first_terminated = NULL;

extern void thread_prepare_startup(
    thread_t *t,
    void (*entry_point)(void));

static void _thread_cleanup_terminated(void)
{
    thread_t *t = thread_first_terminated;

    while (t) {
        thread_t *next = t->hdr.next;
        void *proc = t->process;

        /* cannot reclaim the stack currently used by the interrupt/scheduler */
        if (t == thread_current) {
            t = next;
            continue;
        }

        mem_free_owner((void *)&_heap, (void *)t);
        so_destroy((void **)&thread_first_terminated, (void *)t);
        process_reap((process_t *)proc);

        t = next;
    }
}

/* TODO: add parent process */
thread_t *thread_create(
    void (*entry_point)(void),
    uint16_t stack_size,
    void *process)
{
    thread_t *t;
    void *stack;

    /* we can't be bothered while creating new thread */
    enter_critical_section();

    if (t = (thread_t *)so_create((void **)&thread_first_suspended,
                                  sizeof(thread_t), NONE))
    { 
        /* TODO: owner is process so allocate stack from the process */
        stack = mem_allocate((void *)&_heap, stack_size, (void *)t);
        if (!stack)
        {
            /* was allocated - so free it */
            so_destroy((void **)&thread_first_suspended, (void *)t);
            t = NULL;
        }
        else
        {
            t->wait = NULL;
            t->state = THREAD_STATE_SUSPENDED;
            t->process = process; /* parent */
            /* prepare stack */
            t->sp = (uint16_t)stack + stack_size - CONTEXT_SIZE;
            /* emit startup code and seed the initial return address */
            thread_prepare_startup(t, entry_point);

            /* and that's it. when scheduler yields 
               a slice to this thread it will execute ret
               which will jump to startup code */
        }
    }

    /* we're done */
    leave_critical_section();

    return t;
}

void _thread_lswitch(
    thread_t **src, 
    thread_t **dst, 
    thread_t *t,
    uint8_t state,
    bool immediate) 
{
    /* make sure we are not bothered while moving
       threads around */
    enter_critical_section();

    /* move thread from suspended queue to running queue */
	if (list_remove(
        (list_item_t**)src, 
        (list_item_t *)t)!=NULL) {
            list_insert(
                (list_item_t**)dst, 
                (list_item_t *)t);
            t->state=state;
        }

    /* leave critical section, we're done! */
    leave_critical_section();

    /* if switch should be immediate, then simply halt the
       current process, interrupt will release it! */
    if (immediate) __asm__("halt");
}

void thread_resume(thread_t *t) {
    _thread_lswitch(
        &thread_first_suspended,
        &thread_first_running,
        t,
        THREAD_STATE_RUNNING,
        false
    );
}

void thread_suspend(thread_t *t) {
    _thread_lswitch(
        &thread_first_running,
        &thread_first_suspended,
        t,
        THREAD_STATE_SUSPENDED,
        true
    );
}

void thread_exit(thread_t *t)
{
    _thread_lswitch(
        &thread_first_running,
        &thread_first_terminated,
        t,
        THREAD_STATE_TERMINATED,
        true
    );
    /* safety net: terminated thread must never run arbitrary fall-through. */
    while (1) __asm__("halt");
}

/* select next thread to run */
thread_t* _thread_select_next(void) {

    thread_t *curr, *t;

    /* periodic reap of terminated threads and their owned resources */
    _thread_cleanup_terminated();

	/* release any waiting task */
	curr=thread_first_waiting;
	while (curr) {
		t=curr;
		curr=curr->hdr.next;
        /* test thread for signaled events */
        uint8_t sig=0;
        for (uint8_t i=0;i<t->num_events;i++)
		    if (((t->wait)[i])->state==signaled) {
                sig=1;
                break;
            }
		if (sig) {	
			/* move task from running to waiting list... */
			t->state = THREAD_STATE_RUNNING;
			list_remove((list_item_t**)&thread_first_waiting, (list_item_t *)t);
			list_insert((list_item_t**)&thread_first_running, (list_item_t *)t);
		} 
	}

    /* select next thread to run */
    if (thread_current==NULL 
        || thread_current->state!=THREAD_STATE_RUNNING
        || (thread_current->hdr).next==NULL) 
        /* if there is no current thread, or current thread is
           not running anymore, or we are at the end of running
           list, then take first running thread...
           ...could also be NULL */
        return thread_first_running;
    else if ((thread_current->hdr).next!=NULL)
        /* take next thread */
        return (thread_current->hdr).next;
    else
        /* no change */
        return thread_current;

}
