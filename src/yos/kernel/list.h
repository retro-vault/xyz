/*
 * list.c
 *
 * single linked list
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-23   tstih
 *
 */
#ifndef __LIST_H__
#define __LIST_H__

#include <stdint.h>
#include <stddef.h>

/* each linked list must start with list_header */
typedef struct list_item_s
{
    void *next; /* next list el. */
} list_item_t;

/* match functions */
extern uint8_t list_match_eq(list_item_t *p, uint16_t arg);

/* functions */
extern list_item_t *list_find(
    list_item_t *first,
    list_item_t **prev,
    uint8_t (*match)(list_item_t *p, uint16_t arg),
    uint16_t the_arg);
extern void list_iterate(
    list_item_t *first,
    void((*fn)(list_item_t *p, uint16_t arg)),
    uint16_t the_arg);

/* core functions */
extern list_item_t *list_insert(list_item_t **first, list_item_t *el);
extern list_item_t *list_append(list_item_t **first, list_item_t *el);
extern list_item_t *list_remove(list_item_t **first, list_item_t *el);
extern list_item_t *list_remove_first(list_item_t **first);

#endif /* __LIST_H__ */