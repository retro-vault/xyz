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

#include <drivers/mdr.h>

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
    int (*ver)(void);                   /* api version */
    
    /* critical sections (refcounted) */
    void (*enter_critical_section)(void);
    void (*leave_critical_section)(void);

    /* TODO: timers */
    handle (*install_timer)(void (*handler)(void), int ticks);
    void (*uninstall_timer)(handle timer);

    /* events */

    /* threads */

    /* standard library - stdio.h */
    void (*printf)(const char *format, ...);
    void (*puts)(const char *s);
    void (*gets)(char *s);
    
    /* standard library - conio.h */
    void (*clrscr)(void);               /* clear screen */
    int (*kbhit)(void);                 /* check for key, no blocking */
    void (*setcur)(int enable);         /* enable/disable cursor */
    void (*setattr)(unsigned char attr);

    /* TODO: standard library - mem.h */
    void *(*malloc)(unsigned int size);
    void (*free)(void *p);

    /* standard library - time.h */
    unsigned int (*clock)(void);

    /* storage - microdrive */
    uint8_t (*mdr_detect_drives)(void);
    uint8_t (*mdr_format)(uint8_t drive, char *cart_name);
    uint8_t (*mdr_dir)(uint8_t drive, mdr_file_t *files, uint8_t max);
    uint8_t (*mdr_load)(uint8_t drive, char *name, uint8_t *dest);
    uint8_t (*mdr_save)(uint8_t drive, char *name, uint8_t *src, uint16_t len);

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
