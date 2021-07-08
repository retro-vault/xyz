/*
 * service.h
 *
 * a service is a table of function pointers
 * accessible via service name.
 * yos syscalls are implemented as functions
 * of a service. each process can register its
 * own service(s). operating system calls are
 * available via the "yos" service.
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-08   tstih
 *
 */
#ifndef __SERVICE_H__
#define __SERVICE_H__

#include <sysobj.h>
#include <mem.h>
#include <string.h>

#define MAX_SVC_NAME_LEN    16

/* the service structure */
typedef struct service_s {
    sysobj_t hdr;                       /* timer is a sysobj */
    char name[MAX_SVC_NAME_LEN];        /* service name */
    void *fntable;                      /* function table */
} service_t;

/* function to query a service. the operating system
   registers this service under RST10 call, it returns
   a pointer to the OS api */
extern void *_svc_query(const char *name);

/* register a service */
extern service_t* svc_register(const char *name, void *fntable);

/* unregister a service */
extern void svc_unregister(service_t *s);

#endif /* __SERVICE_H__ */