/*
 * Declares the base YOS system-object header used for owned kernel
 * resources that participate in intrusive tracking lists.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __SYSOBJ_H__
#define __SYSOBJ_H__

#include <kernel/list.h>

/*
 * System-owned allocator heap supplied by the kernel.
 */
extern void *_sys_heap;
#define __sys_heap _sys_heap

/*
 * Base header shared by all tracked YOS system objects.
 */
typedef struct sysobj_s {
    union {
        list_item_t hdr;
        void* next;                     /* binary compatible with list_item_t */
    };
    void* owner;                        /* owners' id */
} sysobj_t;

/*
 * Allocate one system object and link it into the supplied list.
 */
void *so_create(void **first, uint16_t size, void *owner);

/*
 * Unlink one system object from the supplied list and free it.
 */
void *so_destroy(void **first, void *so);

#endif /* __SYSOBJ_H__ */
