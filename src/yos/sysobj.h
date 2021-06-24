/*
 * sysobj.h
 *
 * every system resource in yos is an object
 * and is bookkeeped by the os.  
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-23   tstih
 *
 */
#ifndef __SYSOBJ_H__
#define __SYSOBJ_H__

#include <list.h>

/* good lord (=the operating system) will provide */
extern void *sys_heap;

/* sysobject is binary compatible with list header */
typedef struct sysobj_s {
    union {
        list_item_t hdr;
        void* next;                     /* binary compatible with list_item_t */
    };
    void* owner;                        /* owners' id */
} sysobj_t;

/* allocate memory for system object and add it to list */
void *so_create(void **first, uint16_t size, void *owner);

/* remove system object from list and free its memory */
void *so_destroy(void **first, void *so);

#endif /* __SYSOBJ_H__ */