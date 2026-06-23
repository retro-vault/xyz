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
#include <kernel/list.h>

#define LIST_GUARD_MAX 1024

/*
 * check if arg and pointer are equal
 */
uint8_t list_match_eq(list_item_t *p, uint16_t arg)
{
    return (((uint16_t)p) == arg);
}


/*
 * iterate linked list. call fn for every member, passing
 * it the pointer and the argument
 */
void list_iterate(
    list_item_t *first,
    void ((*fn)(list_item_t *p, uint16_t arg)),
    uint16_t the_arg)
{
    list_item_t *slow = first;
    list_item_t *fast = first;
    uint16_t guard = 0;
    while (first)
    {
        fn(first, the_arg);
        first = first->next;
        guard++;
        if (guard >= LIST_GUARD_MAX) break; /* probable corruption */
        if (slow) slow = (list_item_t *)slow->next;
        if (fast && fast->next)
            fast = (list_item_t *)((list_item_t *)fast->next)->next;
        else fast = NULL;
        if (slow && fast && slow == fast) break; /* cycle detected */
    }
}

/*
 * find element using match function
 * this can be used for all sorts of tasks, for example if you want to find last element
 * you seek for element which has element->next==NULL
 */
list_item_t *list_find(
    list_item_t *first,
    list_item_t **prev,
    uint8_t((*match)(list_item_t *p, uint16_t arg)),
    uint16_t the_arg)
{
    uint16_t guard = 0;
    /* assume no previous */
    *prev = NULL;
    while (first && !match(first, the_arg))
    {
        *prev = first;
        first = first->next;
        guard++;
        if (guard >= LIST_GUARD_MAX) return NULL; /* probable cycle/corruption */
    }
    /* result is in first */
    return first;
}

/*
 * insert element into linked list at begining
 */
list_item_t *list_insert(list_item_t **first, list_item_t *el)
{
    el->next = *first;
    *first = el;
    return el;
}

/*
 * insert element into linked list at end
 */
list_item_t *list_append(list_item_t **first, list_item_t *el)
{

    list_item_t *current;
    uint16_t guard = 0;

    el->next = NULL; /* it's always the last element */

    if (*first == NULL) /* empty list? */
        *first = el;
    else
    {
        current = *first;
        while (current->next)
        {
            current = current->next;
            guard++;
            if (guard >= LIST_GUARD_MAX) return NULL; /* probable cycle/corruption */
        }
        current->next = el;
    }
    return el;
}

/*
 * removes element from linked list
 */
list_item_t *list_remove(list_item_t **first, list_item_t *el)
{
    list_item_t *prev;
    if (el == *first)
    {
        *first = el->next;
    }
    else
    {
        if (!list_find(*first, &prev, list_match_eq, (uint16_t)el))
            return NULL;
        else /* rewire */
            prev->next = el->next;
    }
    return el;
}

/*
 * remove first element from linked list
 */
list_item_t *list_remove_first(list_item_t **first)
{
    list_item_t *result;
    if (*first == NULL)
        return NULL; /* empty list */
    result = *first;
    *first = (list_item_t *)((*first)->next);
    return result;
}
