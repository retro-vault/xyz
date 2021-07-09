/*
 * service.c
 *
 * a service is a table of function pointers
 * accessible via service name.
 * yos syscalls are implemented as functions
 * of a service. each process can register its
 * own service(s). operating system calls are
 * available via the "yos" service.
 * 
 * TODO:
 *  add owner
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-08   tstih
 *
 */
#include <service.h>

service_t *_svc_first=NULL;

uint8_t _svc_match_eq(list_item_t *p, uint16_t arg) {
    char *name=(char *)arg;
    service_t *s=(service_t *)p;
    return ( strcmp (name,s->name)==0 );
}

service_t* svc_register(const char *name, void *service) {
    service_t *s;
	if ( s = (service_t *)so_create(
            (void **)&_svc_first, sizeof(service_t), NONE
        ) 
    ) {
		strcpy(s->name,name);
        s->fntable=service;
	}
	return s;
}

void svc_unregister(service_t *s) {
    so_destroy((void **)&_svc_first, (void *)s);
}

void *_svc_query(const char *name) {
    list_item_t *prev;
    service_t *s = (service_t *)list_find(
        (list_item_t *)_svc_first,
        &prev,
        _svc_match_eq,
        (uint16_t) name);
    if (s!=NULL) return s->fntable;
    else return NULL;
}