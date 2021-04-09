/*
 *	service.c
 *	in yos every api is a service, even yos itself. you obtain a pointer to
 *  a service by calling service_query and then you call service functions
 *  through it.
 *
 *	tomaz stih thu apr 9 2021
 */
#include "yos.h"
#include "service.h"

/* pointer to the first service in linked list */
service_t* service_first;

/* return a pointer to your service function table */
void *service_query(char *name) {

}

/* register your own api so that it can be accessed from yos api */
void service_register(void *name, addr_t api) {

}

/* unregister your service */
void service_unregister(void *name) {

}