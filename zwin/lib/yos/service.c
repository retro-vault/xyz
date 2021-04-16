/*
 * service.c
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
#include "yos.h"
#include "service.h"

/* pointer to the first service in linked list */
service_t* service_first=NULL;

/* return a pointer to your service function table */
byte_t service_match(list_header_t *p, addr_t arg) {
    return !string_compare(((service_t*)p)->name,arg);
}

void *service_query(char *name) {
    service_t *prev, *curr;
    curr=list_find(
        service_first,
        &prev,
        service_match,
        name);
    if (curr==NULL)
        return NULL;
    else
        return curr->svc_ptr;
}

/* register your own api so that it can be accessed from yos api */
void service_register(void *name, addr_t api, handle_t owner) {
    /* create new service object */
    service_t *svc=mem_allocate(yos_heap,sizeof(service_t), owner);
    /* store pointer to function table */
    svc->svc_ptr=api;
    /* create name */
    svc->name=mem_allocate(yos_heap,string_length(name)+1, owner);
    string_copy(svc->name,name);
    /* and add to linked list of services */
    list_insert(&service_first, svc);
}

/* unregister your service */
void service_unregister(void *name) {
    /* TODO: unregister service */
}