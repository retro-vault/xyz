/*
 * timer.c
 *
 * timer management for xyz
 * TODO:
 *  make install and uninstall thread safe
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-23   tstih
 *
 */
#include <timer.h>

timer_t *_tmr_first=NULL;

/* 
 * install timer hook 
 */
timer_t *tmr_install(void (*hook)(), uint16_t ticks, void *owner) {
	timer_t *t;
	if ( t = (timer_t *)so_create(
            (void **)&_tmr_first, sizeof(timer_t), owner
        ) 
    ) {
		t->hook=hook;
		t->_tick_count=t->ticks=ticks;	
	}
	return t;
}

/*
 * remove timer hook
 */
timer_t *tmr_uninstall(timer_t *t) {
	return (timer_t *)so_destroy((void **)&_tmr_first, (void *)t);
}

/*
 * chain timers
 * note: this code is called inside (already!) guarded 
 * 50 hz interrupt so no extra di/ei calls are needed.
 */
void _tmr_chain() {
	timer_t *t=_tmr_first;
	while(t) {
		if (t->_tick_count==0) { /* trig it */
			t->_tick_count=t->ticks;			
			t->hook();
		} else t->_tick_count--;
		t=t->hdr.next;
	}
}