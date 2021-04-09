/*
 *	message.c
 *	message
 *
 *	tomaz stih sun oct 27 2013
 */
#include "window.h"
#include "message.h"

message_t *message_first;

/* initialize messaging system */
void message_init() {
	message_first=NULL;
}