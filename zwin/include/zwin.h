/*
 * zwin.h
 * z windows single header file.  
 *
 * tomaz stih tue mar 23 2021
 */
#ifndef _ZWIN_H
#define _ZWIN_H

#define ZWIN_API "zwin"

/* zwin api */
struct zwin_s {

    /* memory management functions */
    void * (*malloc)();
    void (*free)();

} zwin_t;

#endif /* _ZWIN_H */