/*
 * zwin.h
 *
 * z windows header file
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 23.03.2021   tstih
 *
 */

/*
 * zwin.h
 * z windows single header file.  
 *
 * tomaz stih tue mar 23 2021
 */
#ifndef _ZWIN_H
#define _ZWIN_H

#include "yos.h"
#include "gpx.h"

#define ZWIN_API "zwin"


/*
 * z windows structures 
 */

/* window */
typedef struct window_s window_t;
struct window_s
{
    window_t *next;
    word_t reserved;
    struct window_s *parent;
    struct window_s *first_child;
    byte_t flags;
    rect_t *rect;
    /* TODO: gpx pointer */
    result_t((*wnd_proc)(
        window_t *wnd, 
        byte_t id, 
        word_t param1, 
        word_t param2));
    string_t title;
};


/* 
 * zwindosw api 
 */
struct zwin_s {
    result_t (*sendmsg)(
        window_t *wnd, 
        byte_t id, 
        word_t param1, 
        word_t param2);
    result_t (*postmsg)(
        window_t *wnd, 
        byte_t id, 
        word_t param1, 
        word_t param2);
} zwin_t;

#endif /* _ZWIN_H */