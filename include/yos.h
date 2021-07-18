/*
 * yos.h
 *
 * yos syscalls (yos API)
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-09   tstih
 *
 */
#ifndef __YOS_H__
#define __YOS_H__

#define YOS_VERSION 0x03

#define	RST08   0
#define	RST10   1
#define	RST18   2
#define	RST20   3
#define	RST28   4
#define	RST30   5
#define	RST38   6
#define NMI	    7

#define AT_NONE         0x00
#define AT_UNDERLINE    0x01
#define AT_INVERSE      0x02

typedef void * handle;

typedef struct yos_s {

    /* convention */
    int (*ver)();                       /* api version */
    
    /* interrupts */
    void (*di)();                       /* di with ref counting */
    void (*ei)();                       /* ei with ref counting */

    /* TODO: timers */
    handle (*install_timer)(void (*handler)(), int ticks);
    void (*uninstall_timer)(handle timer);

    /* events */

    /* threads */

    /* standard library - stdio.h */
    void (*printf)(const char *format, ...);
    void (*puts)(const char *s);
    void (*gets)(char *s);
    
    /* standard library - conio.h */
    void (*clrscr)();                   /* clear screen */
    int (*kbhit)();                     /* check for key, no blocking */
    void (*setcur)(int enable);         /* enable/disable cursor */
    void (*setattr)(unsigned char attr);

    /* TODO: standard library - mem.h */
    void *(*malloc)(unsigned int size);
    void (*free)(void *p);

    /* standard library - time.h */
    unsigned int (*clock)();

    /* standard library - string.h */
    unsigned int (*strlen)(const char *s);
    char* (*strcpy)(char *d, const char *s);
    int (*strcmp)(const char *s1, const char *s2);

    /* standard library - ctype.h */
    int (*isalpha)(int c);
    int (*isspace)(int c);
    int (*tolower)(int c);

} yos_t;

/* get the api by name, for syscalls use "yos" */
extern void *query_interface(char *name);

#endif /* __YOS_H__ */