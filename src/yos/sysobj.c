/*
 * sysobj.c
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
#include <sysobj.h>
#include <mem.h>

void *so_create(void **first, uint16_t size, void *owner) {
	sysobj_t *p;
	if( p = (sysobj_t *)mem_allocate(&_sys_heap, size, owner) ) {
		list_insert((list_item_t**)first, (list_item_t *)p);
		p->owner=owner;
	}
	return (void *)p;
}

void *so_destroy(void **first, void *e) {
	if ( e = list_remove( (list_item_t **)first, (list_item_t *)e) ) {
		e = mem_free(&_sys_heap, e);
	}
	return e;
}