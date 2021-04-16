/*
 * yos.h
 *
 * the yos api. include this into programs that use the yos system calls.
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 07.04.2021   tstih
 *
 */
#ifndef _YOS_H
#define _YOS_H

/* yos types */
#if __LINUX_SDL2__
#include <stdint.h>
typedef uint8_t     byte_t;
typedef uint16_t    word_t;
typedef byte_t      boolean_t;
typedef uint8_t *   string_t;
typedef byte_t      result_t;
typedef void *      addr_t;
typedef int         addr_size_t;
typedef addr_t      handle_t;
#elif __ID_PARTNER__

#elif __ZX_SPECTRUM__

#endif

/* yos definitions */
#ifndef NULL
#define NULL ((addr_t)0)
#endif

#ifndef TRUE
#define TRUE ((byte_t)1)
#endif

#ifndef FALSE
#define FALSE ((byte_t)0)
#endif

/* API */
#define YOS_API "yos"

typedef struct yos_s
{
    /* memory allocation */
    void *(*malloc)(addr_size_t size);
    void (*free)(void *);

    /* string */
    addr_size_t (*strlen)(char *str);
    char *(*strcpy)(char *dest, const char *src);
    int (*strcmp)(char *s1, char *s2);

    /* system lists */
	void* (*lappend)(void **first, void *el);
	void* (*linsert)(void **first, void *el);
	void* (*lremove)(void **first, void *el);
	void* (*lremfirst)(void **first);

    /* system wide errors */
    result_t (*geterr)();
    result_t (*seterr)(result_t err);

} yos_t;

/* error codes */
#define SUCCESS         0
#define OUT_OF_MEMORY   1

/* call this to obtain the yos interface
 *   yos_t* api=(yos*)query_api("yo00"); // 2 letters + 2 dig. version
 *   void* mem_block=api->malloc(1024);
 * this function is usually defined in crt0.s for yos application
 * and follows standard protocol (such as RST 0x10 call) to obtain the address.
 */
void *query_api(char *name);

#endif /* _YOS_H */