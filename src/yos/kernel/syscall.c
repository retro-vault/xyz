/*
 * syscall.c
 *
 * yos syscalls (yos API)
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-09   tstih
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <kernel/interrupts.h>
#include <kernel/mem.h>

#include <tty/tty.h>
#include <tty/tty_print.h>
#include <drivers/mdr.h>

#include <yos.h>

yos_t _yos;

int yos_version(void) { return YOS_VERSION; }
extern unsigned int _clock(void);

static void *yos_malloc(unsigned int size) {
    return mem_allocate((void *)&_heap, (uint16_t)size, NONE);
}

static void yos_free(void *p) {
    (void)mem_free((void *)&_heap, p);
}

static unsigned int yos_clock(void) {
    return (unsigned int)_clock();
}

/* populate function list */
yos_t* _yos_init(void) {

    /* core */
    _yos.ver=yos_version;
    _yos.enter_critical_section=enter_critical_section;
    _yos.leave_critical_section=leave_critical_section;
    
    /* stdio.h */
    _yos.printf=tty_printf;
    _yos.puts=tty_puts;
    _yos.gets=tty_gets;
    
    /* conio.h */
    _yos.clrscr=tty_cls;
    _yos.kbhit=tty_getc;
    _yos.setcur=tty_cur_enable;
    _yos.setattr=tty_attr;

    /* stdlib memory/time */
    _yos.malloc=yos_malloc;
    _yos.free=yos_free;
    _yos.clock=yos_clock;

    /* microdrive */
    _yos.mdr_detect_drives=mdr_detect_drives;
    _yos.mdr_format=mdr_format;
    _yos.mdr_dir=mdr_dir;
    _yos.mdr_load=mdr_load;
    _yos.mdr_save=mdr_save;
    
    /* string.h */
    _yos.strlen=strlen;
    _yos.strcpy=strcpy;
    _yos.strcmp=strcmp;

    /* ctype.h */
    _yos.isalpha=isalpha;
    _yos.isspace=isspace;
    _yos.tolower=tolower;

    /* and return fntable */
    return &_yos;
}
