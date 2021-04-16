/*
 * yos.c
 *
 * yos core functions
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 13.04.2021   tstih
 *
 */
#include "yos.h"
#include "memory.h"
#include "string.h"
#include "service.h"

#if __LINUX_SDL2__
#include <stdlib.h>
#endif

/* pointer to heap */
addr_t yos_heap, usr_heap;

/* last error */
result_t last_error;

/*
 * api functions
 */
void *yos_malloc(addr_size_t size) {
    /* TODO: use current process id as the owner */
    return mem_allocate(usr_heap,size,NULL);
}

void yos_free(void *p) {
    mem_free(usr_heap,p);
}

result_t yos_geterr() {
    return last_error;
}

result_t yos_seterr(result_t err) {
    last_error=err;
    return last_error;
}

/*
 * api registration and os initialization 
 */
void yos_init() {

    /* initialize memory */
#if __LINUX_SDL2__
    yos_heap=malloc(0x0800); /* 2KB OS heap */
    usr_heap=malloc(0x8000); /* 32KB user heap */
#endif
    /* TODO: heap size and position as external parameter */
    mem_init(yos_heap,0x0800);
    mem_init(usr_heap,0x8000); 

    /* create table of functions */
    yos_t yapi;
    yapi.malloc=yos_malloc;
    yapi.free=yos_free;
    yapi.strcmp=string_compare;
    yapi.strcpy=string_copy;
    yapi.strlen=string_length;
    yapi.geterr=yos_geterr;
    yapi.seterr=yos_seterr;
    yapi.lappend=list_append;
    yapi.linsert=list_insert;
    yapi.lremove=list_remove;
    yapi.lremfirst=list_remove_first;

    /* register api */
    service_register(YOS_API, &yapi, NULL);

    /* set last error to SUCCESS */
    last_error=SUCCESS;
}


/*
 * query_api
 */
void *query_api(char *name) { /* TODO: mmove to crt0 */
    return service_query(name);
}