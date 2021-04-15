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