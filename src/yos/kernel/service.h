/*
 * Declares the service registry used by YOS to expose function tables by
 * name, including the main `"yos"` kernel service.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __SERVICE_H__
#define __SERVICE_H__

#include <string.h>

#include <kernel/sysobj.h>
#include <kernel/mem.h>

/*
 * Maximum service-name length including the terminating NUL.
 */
#define MAX_SVC_NAME_LEN    16

/*
 * Registered named service entry.
 */
typedef struct service_s {
    sysobj_t hdr;                       /* timer is a sysobj */
    char name[MAX_SVC_NAME_LEN];        /* service name */
    void *fntable;                      /* function table */
} service_t;

/*
 * Head of the global service list.
 */
extern service_t *_svc_first;

/*
 * Look up a registered service table by name.
 */
extern void *_svc_query(const char *name);
/*
 * RST10 entry point wrapper around `_svc_query()`.
 */
extern void svc_query_rst10(void);

/*
 * Register one service name and function table.
 */
extern service_t* svc_register(const char *name, void *fntable);

/*
 * Unregister and destroy one service entry.
 */
extern void svc_unregister(service_t *s);

#endif /* __SERVICE_H__ */
