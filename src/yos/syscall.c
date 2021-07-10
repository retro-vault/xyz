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
#include <interrupts.h>
#include <tty.h>
#include <tty_print.h>
#include <yos.h>

yos_t _yos;

int yos_version() { return 0x10; }

/* populate function list */
yos_t* _yos_init() {

    /* core */
    _yos.ver=yos_version;
    _yos.ei=ir_enable;
    _yos.di=ir_disable;
    
    /* stdio.h */
    _yos.printf=tty_printf;
    _yos.puts=tty_puts;
    _yos.gets=tty_gets;
    
    /* conio.h */
    _yos.clrscr=tty_cls;
    _yos.kbhit=tty_getc;
    
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