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
#include <kernel/evt.h>

event_t *_evt_first=NULL;

/* creates new event, adds to the list of events */
[[sdcc::naked]] event_t *evt_create(void *owner) {
    owner;
    __asm__(
        "push hl\n"
        "ld de, #5\n"
        "ld hl, #__evt_first\n"
        "call _so_create\n"
        "ld a, e\n"
        "or d\n"
        "ret z\n"
        "ld hl, #4\n"
        "add hl, de\n"
        "ld (hl), #0\n"
        "ret\n");
}

/* destroys existing event, removes from the list of events */
[[sdcc::naked]] event_t *evt_destroy(event_t *e) {
    e;
    __asm__(
        "ex de, hl\n"
        "ld hl, #__evt_first\n"
        "jp _so_destroy\n");
}

/* set event state */
event_t *evt_set(event_t *e, event_state_t newstate) {
    newstate;
	list_item_t *prev=NULL;
	if ( e = (event_t *)list_find(
        (list_item_t *)_evt_first, 
        &prev, 
        list_match_eq, 
        (uint16_t)e) ) e->state=newstate;
	return e;
}
