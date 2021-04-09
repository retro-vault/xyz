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
typedef signed char byte_t;
typedef int         word_t;
typedef byte_t      boolean_t;
typedef char *      string_t;
typedef byte_t      result_t;
typedef void *      addr_t;
typedef int         addr_size_t;
typedef addr_t      handle_t;


/* yos definitions */
#ifndef NULL
#define NULL ((addr_t)0)
#endif

#ifndef TRUE
#define TRUE ((byte)1)
#endif

#ifndef FALSE
#define FALSE ((byte)0)
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