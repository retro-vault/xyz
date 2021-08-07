/*
 * thread.c
 *
 * threading logic
 * TODO: guard ops with di/ei
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-23   tstih
 *
 */
#include <kernel/thread.h>

thread_t *thread_current = NULL;
thread_t *thread_first_suspended = NULL;
thread_t *thread_first_running = NULL;
thread_t *thread_first_waiting = NULL;
thread_t *thread_first_terminated = NULL;

extern uint8_t lob(uint16_t w) __naked;
extern uint8_t hib(uint16_t w) __naked;

/* TODO: add parent process */
thread_t *thread_create(
    void (*entry_point)(),
    uint16_t stack_size,
    void *process)
{
    thread_t *t;
    void *stack;

    /* we can't be bothered while creating new thread */
    ir_disable();

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
            /* emit startup code: */
            /*  call entry_point */
            (t->startup)[0] = 0xcd;     /* call opcode */
            (t->startup)[1] = lob((uint16_t)entry_point);
            (t->startup)[2] = hib((uint16_t)entry_point);
            /*  ld hl,t */
            (t->startup)[3] = 0x21;     /* ld hl, opcode */
            (t->startup)[4] = lob((uint16_t)t);
            (t->startup)[5] = hib((uint16_t)t);
            /*  push hl */
            (t->startup)[6] = 0xe5;     /* push hl opcode */
            /*  jp __thread_exit */
            (t->startup)[7] = 0xc3;     /* jp opcode */
            (t->startup)[8] = lob((uint16_t)&thread_exit);
            (t->startup)[9] = hib((uint16_t)&thread_exit);

            /* top two bytes are the return address,
               make them point to startup code */
            uint16_t *ret_addr = (uint16_t *)(t->sp + CONTEXT_SIZE - 2);
            (*ret_addr) = (uint16_t)(&(t->startup));

            /* and that's it. when scheduler yields 
               a slice to this thread it will execute ret
               which will jump to startup code */
        }
    }

    /* we're done */
    ir_enable();

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
    __asm__("di");

    /* move thread from suspended queue to running queue */
	if (list_remove(
        (list_item_t**)src, 
        (list_item_t *)t)!=NULL) {
            list_insert(
                (list_item_t**)dst, 
                (list_item_t *)t);
            t->state=state;
        }

    /* enable interrupts, we're done! */
    __asm__("ei");

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
}

/* select next thread to run */
thread_t* _thread_select_next() {

    thread_t *curr, *t;
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

/* low byte service function */
uint8_t lob(uint16_t w) __naked {
    w;
    __asm 
        pop     de                      ; get ret address
        pop     hl                      ; get word
        ld      h,#0                    ; hi byte to 0
        push    hl                      ; restore...
        push    de                      ; ...stack
        ret
    __endasm; 
}

/* high byte service function */
uint8_t hib(uint16_t w) __naked {
    w;
    __asm 
        pop     de                      ; get ret address
        pop     hl                      ; get word
        ld      l,h                     ; high to low
        ld      h,#0                    ; hi byte to 0
        push    hl                      ; restore...
        push    de                      ; ...stack
        ret
    __endasm; 
}