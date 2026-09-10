/*
 * Declares the intrusive singly linked-list helpers shared across the YOS
 * kernel object model.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2021 tomaz stih
 */
#ifndef __LIST_H__
#define __LIST_H__

#include <stdint.h>
#include <stddef.h>

/*
 * Intrusive list node header that prefixes each list element.
 */
typedef struct list_item_s
{
    void *next;         /* next list el. */
    uint8_t data[0];    /* list item data from here... */
} list_item_t;

/*
 * Match helper that compares one element argument for equality.
 */
extern uint8_t list_match_eq(list_item_t *p, uint16_t arg);

/*
 * Find the first element for which the callback returns true.
 */
extern list_item_t *list_find(
    list_item_t *first,
    list_item_t **prev,
    uint8_t (*match)(list_item_t *p, uint16_t arg),
    uint16_t the_arg);
/*
 * Visit each element in the list with a caller callback.
 */
extern void list_iterate(
    list_item_t *first,
    void((*fn)(list_item_t *p, uint16_t arg)),
    uint16_t the_arg);

/*
 * Insert one element at the list head.
 */
extern list_item_t *list_insert(list_item_t **first, list_item_t *el);
/*
 * Append one element to the tail of the list.
 */
extern list_item_t *list_append(list_item_t **first, list_item_t *el);
/*
 * Remove one explicit element from the list.
 */
extern list_item_t *list_remove(list_item_t **first, list_item_t *el);
/*
 * Remove and return the first element of the list.
 */
extern list_item_t *list_remove_first(list_item_t **first);

#endif /* __LIST_H__ */
