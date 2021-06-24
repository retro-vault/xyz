/*
 * evt.c
 *
 * sync. event can be in signalled or non-signalled
 * state and is used by the scheduler to block threads
 * that are waiting for it.
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-23   tstih
 *
 */
#include <evt.h>

event_t *evt_first=NULL;

/* creates new event, adds to the list of events */
event_t *evt_create(void *owner) {
	event_t *e;
	if ( e = (event_t *)so_create((void **)&evt_first, sizeof(event_t), owner) )
		e->state=nonsignaled;
	return e;
}

/* destroys existing event, removes from the list of events */
event_t *evt_destroy(event_t *e) {
	return (event_t *)so_destroy((void **)&evt_first, (void *)e);	
}

/* set event state */
event_t *evt_set(event_t *e, event_state_t newstate) {
    newstate;
	list_item_t *prev=NULL;
	if ( e = (event_t *)list_find(
        (list_item_t *)evt_first, 
        &prev, 
        list_match_eq, 
        (uint16_t)e) ) e->state=newstate;
	return e;
}