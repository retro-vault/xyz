/*
 * evt.h
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
#ifndef __EVT_H__
#define __EVT_H__

#include <stdint.h>
#include <sysobj.h>

typedef enum event_state_e {
	nonsignaled,
	signaled
} event_state_t;

typedef struct event_s {
	/* event is a system object */
	sysobj_t hdr;
	/* event state */
	event_state_t state;
} event_t;

extern event_t *_evt_first;

extern event_t *evt_create(void *owner);
extern event_t *evt_destroy(event_t *e);
extern event_t *evt_set(event_t *e, event_state_t newstate);

#endif /* __EVT_H__ */