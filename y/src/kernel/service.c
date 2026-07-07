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
#include <kernel/service.h>

service_t *_svc_first=NULL;

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

[[sdcc::naked]] void svc_unregister(service_t *s) {
    s;
    __asm__(
        "ex de, hl\n"
        "ld hl, #__svc_first\n"
        "jp _so_destroy\n");
}

void *_svc_query(const char *name) {
    service_t *s = _svc_first;
    uint8_t guard = 0;

    while (s) {
        if (strcmp(name, s->name) == 0) {
            return s->fntable;
        }
        s = (service_t *)s->hdr.next;
        if (++guard == 0) break;
    }

    return NULL;
}

/* RST10 bridge:
 *   input : HL = service name pointer
 *   output: DE = service table pointer (or 0)
 */
[[sdcc::naked]] void svc_query_rst10(void) {
    __asm__(
        "call __svc_query\n"
        "ret\n");
}
