/*
 * Declares the yos system call table, shared constants, and
 * service entry points used by hosted applications and drivers.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef __YOS_H__
#define __YOS_H__

#include <drivers/mdr.h>

/* Current yos API version returned by the kernel interface. */
#define YOS_VERSION 0x04

/* Reset vector index for the `RST 08h` service entry. */
#define	RST08   0
/* Reset vector index for the `RST 10h` service entry. */
#define	RST10   1
/* Reset vector index for the `RST 18h` service entry. */
#define	RST18   2
/* Reset vector index for the `RST 20h` service entry. */
#define	RST20   3
/* Reset vector index for the `RST 28h` service entry. */
#define	RST28   4
/* Reset vector index for the `RST 30h` service entry. */
#define	RST30   5
/* Reset vector index for the `RST 38h` service entry. */
#define	RST38   6
/* Interrupt vector index for the non-maskable interrupt entry. */
#define NMI	    7

/* Default text attribute with no extra styling. */
#define AT_NONE         0x00
/* Text attribute flag that enables underline rendering. */
#define AT_UNDERLINE    0x01
/* Text attribute flag that enables inverse video rendering. */
#define AT_INVERSE      0x02

/*
 * Opaque handle returned by kernel-managed resources.
 */
typedef void * handle;

/*
 * yos kernel service table exposed through `query_interface("yos")`.
 */
typedef struct yos_s {

    int (*ver)(void);                             /* Return the implemented yos API version. */
    void (*enter_critical_section)(void);         /* Enter a refcounted critical section. */
    void (*leave_critical_section)(void);         /* Leave a previously entered critical section. */
    handle (*install_timer)(void (*handler)(void), int ticks); /* Install a timer callback. */
    void (*uninstall_timer)(handle timer);        /* Remove a previously installed timer. */
    void (*printf)(const char *format, ...);      /* Print formatted text to the default console. */
    void (*puts)(const char *s);                  /* Write a string followed by a line terminator. */
    void (*gets)(char *s);                        /* Read a line of input into the supplied buffer. */
    void (*clrscr)(void);                         /* Clear the text screen. */
    int (*kbhit)(void);                           /* Check whether a key is waiting without blocking. */
    void (*setcur)(int enable);                   /* Enable or disable the text cursor. */
    void (*setattr)(unsigned char attr);          /* Change the active text attribute bits. */
    void *(*malloc)(unsigned int size);           /* Allocate a heap block of the requested size. */
    void (*free)(void *p);                        /* Release a previously allocated heap block. */
    unsigned int (*clock)(void);                  /* Return the current kernel tick count. */
    uint8_t (*mdr_detect_drives)(void);           /* Detect attached microdrive units. */
    uint8_t (*mdr_format)(uint8_t drive, char *cart_name); /* Format a microdrive cartridge. */
    uint8_t (*mdr_dir)(uint8_t drive, mdr_file_t *files, uint8_t max); /* Read a cartridge directory. */
    uint8_t (*mdr_load)(uint8_t drive, char *name, uint8_t *dest); /* Load a named microdrive file. */
    uint8_t (*mdr_save)(uint8_t drive, char *name, uint8_t *src, uint16_t len); /* Save a named microdrive file. */
    unsigned int (*strlen)(const char *s);        /* Compute the length of a C string. */
    char* (*strcpy)(char *d, const char *s);      /* Copy one C string into another buffer. */
    int (*strcmp)(const char *s1, const char *s2);/* Compare two C strings lexicographically. */
    int (*isalpha)(int c);                        /* Test whether a character is alphabetic. */
    int (*isspace)(int c);                        /* Test whether a character is whitespace. */
    int (*tolower)(int c);                        /* Convert an uppercase character to lowercase. */

} yos_t;

/*
 * Look up a named kernel interface table.
 *
 * Parameters:
 *      name        - Interface name, such as `"yos"`.
 *
 * Returns:
 *      Pointer to the requested interface table, or `NULL` if unavailable.
 */
extern void *query_interface(char *name);

#endif /* __YOS_H__ */
