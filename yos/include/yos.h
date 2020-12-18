/*
 *	yos.h
 *	the complete yos API
 *  include this to write code for yos
 *
 *	tomaz stih jul 26 2015
 *
 */
#ifndef _YOS_H
#define _YOS_H

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
 *   yos_t* api=(yos*)query_interface("yos");
 *   void* mem_block=api->allocate(1024);
 * this function is defined in crt0.s for yos application
 */
extern void *query_interface(string api);

#endif /* _YOS_H */