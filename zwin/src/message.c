/*
 * message.c
 *
 * window message queue
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 15.04.2021   tstih
 *
 */
#include "window.h"
#include "message.h"

message_t *message_first;

/* initialize messaging system */
void message_init() {
	message_first=NULL;
}

/* remove from queue and free space */
void message_destroy(message_t *m) {
	yos->free((void *)m);
}

/* call wnd_proc directly */
result_t message_send(
    window_t *wnd, 
    byte_t id, 
    word_t param1, 
    word_t param2) {
	return wnd->wnd_proc(wnd, id, param1, param2);
}

/* queue message */
result_t message_post(
    window_t *wnd, 
    byte_t id, 
    word_t param1, 
    word_t param2) {
	
	/* create message */
	message_t *m=yos->malloc(sizeof(message_t));
	if (!m)
		return(yos->seterr(OUT_OF_MEMORY));

	/* populate it */
	m->window=wnd;
	m->id=id;
	m->param1=param1;
	m->param2=param2;

	/* add it to the msg queue */
	yos->lappend((void **)&message_first, (void *)m);

	/* we did it! */
	return (yos->seterr(SUCCESS));
}

/* dispatch new events */
void message_dispatch() {

	message_t *m;

	/* no messages? */
	if (message_first==NULL) return; 
	m = yos->lremfirst((void **)&message_first);
	if (m!=NULL) {
		message_send(m->window,m->id,m->param1,m->param2);
		message_destroy(m);
	}
}