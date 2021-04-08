/*
 *  yos.h
 *  the yos API
 *  include this into programs that use the yos system calls.
 *
 *  tomaz stih apr 7 2021
 *
 */
#ifndef _YOS_H
#define _YOS_H

/* yos definitions */
#ifndef NULL
#define NULL    ( (word)0 )
#endif

#ifndef TRUE
#define TRUE    ( (byte)1 )
#endif

#ifndef FALSE
#define FALSE   ( (byte)0 )
#endif

/* yos types */
typedef signed char         byte;
typedef byte                boolean;
typedef byte                result;
typedef void*				addr;
typedef addr                handle;
typedef char*        		string;

/* API */
typedef struct yos_s yos_t;
struct yos_s {

    /* system lists */
    void* (*lappend)(void **first, void *el);
    void* (*linsert)(void **first, void *el);
    void* (*lremove)(void **first, void *el);
    void* (*lremfirst)(void **first);

    /* memory management */
    void* (*allocate)(word size);
    void (*free)(void *p);
    void (*copy)(byte *src, byte *dst, word count);

    /* tasks, events */
    void (*sleep)(word _50);
};

/* call this to obtain the yos interface
 *   yos_t* api=(yos*)query_api("yo00"); // 2 letters + 2 dig. version
 *   void* mem_block=api->allocate(1024);
 * this function is defined in crt0.s for yos application
 */
extern void *query_api(char *api);

/* register your own api so that it can be accessed from yos api */
extern void register_api(char *api, void *name);

#endif /* _YOS_H */
