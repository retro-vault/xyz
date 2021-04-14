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

typedef struct yos_s yos_t;
struct yos_s
{
    void *(*malloc)(addr_size_t size);
    void (*free)(void *);
};

/* call this to obtain the yos interface
 *   yos_t* api=(yos*)query_api("yo00"); // 2 letters + 2 dig. version
 *   void* mem_block=api->malloc(1024);
 * this function is usually defined in crt0.s for yos application
 * and follows standard protocol (such as RST 0x10 call) to obtain the address.
 */
void *query_api(char *name);

#endif /* _YOS_H */