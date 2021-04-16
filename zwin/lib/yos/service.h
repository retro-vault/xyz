/*
 * service.h
 *
 * in yos every api is a service, even yos itself. you obtain a pointer to
 * a service by calling service_query and then you call service functions
 * through it.
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 09.04.2021   tstih
 *
 */
#ifndef _SERVICE_H
#define _SERVICE_H

#include "yos.h"
#include "list.h"
#include "memory.h"
#include "string.h"

/* pointers to heaps */
extern addr_t yos_heap;
extern addr_t usr_heap;

typedef struct service_s {
    list_header_t list; 
    string_t name;
    addr_t svc_ptr;
} service_t;

/* query service pointer */
extern void *service_query(char *name);

/* register your own api so that it can be accessed from yos api */
extern void service_register(void *name, addr_t api, handle_t owner);

/* unregister your service */
extern void service_unregister(void *name);

#endif /* _SERVICE_H */