/*
 * window.h
 *
 * window functions
 *
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 13.04.2021   tstih
 *
 */
#ifndef _WINDOW_H
#define _WINDOW_H

#include "yos.h"
#include "gpx.h"

/* window flags */
#define WF_NONE         0x00
#define WF_HASTITLE     0x01
#define WF_HASBORDER    0x02
#define WF_DESKTOP      0x04

/* window sizes */
#define WMETR_TITLEHEIGHT 12
#define WMETR_BORDERWIDTH 1

/* window structure */
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
    result_t((*wnd_proc)(window_t *wnd, byte_t id, word_t param1, word_t param2));
    string_t title;
};

/* window drawing */
extern window_t *window_desktop;
extern void window_init();

#endif /* _WINDOW_H */
