/*
 *	list.h
 *	linked list
 *
 *	tomaz stih tue jun 5 2012
 */
#ifndef _LIST_H
#define _LIST_H

#include "yos.h"

/* each linked list must start with list_header */
typedef struct list_header_s
{
    addr_t next;
    addr_t owner;
} list_header_t;

/* match functions */
extern byte_t list_match_eq(list_header_t *p, addr_t arg);

/* functions */
extern list_header_t *list_find(
    list_header_t *first,
    list_header_t **prev,
    byte_t (*match)(list_header_t *p, addr_t arg),
    addr_t the_arg);

/* core functions */
extern list_header_t *list_insert(list_header_t **first, list_header_t *el);
extern list_header_t *list_append(list_header_t **first, list_header_t *el);
extern list_header_t *list_remove(list_header_t **first, list_header_t *el);
extern list_header_t *list_remove_first(list_header_t **first);

#endif
