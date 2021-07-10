/*
 * main.c
 *
 * entry point
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-16   tstih
 *
 */
#include <stdbool.h>
#include <mem.h>
#include <vectors.h>
#include <kbd.h>
#include <tty.h>
#include <tty_print.h>
#include <timer.h>
#include <service.h>
#include <ctype.h>
#include <yos.h>

/* must be naked, returns with reti */
void rst38_handler() __naked {
    __asm
        call    _ir_disable
        ;; store all registers
        push    af
        push    bc
        push    de
        push    hl
        push    ix
        push    iy
        ex      af,af'
        push    af
        ex      af,af'
        exx
        push    bc
        push    de
        push    hl
        exx 
        ;; scan keyboard
        call    __kbd_scan
        ;; run timers
        call    __tmr_chain
        ;; restore regs and allow interrupt again
        exx
        pop     hl 
        pop     de
        pop     bc
        exx
        ex      af,af'
        pop     af
        ex      af,af'
        pop     iy
        pop     ix
        pop     hl
        pop     de
        pop     bc
        pop     af
        call    _ir_enable
        reti
    __endasm;
}

extern yos_t* _yos_init();
extern void shell();

void main() {

    /* create system and user heap  */
    mem_init((void *)&_sys_heap,1024);
    mem_init((void *)&_heap,0xffff-&_sys_heap);

    /* install cursor timer */
    tmr_install(_tty_cur_tick, 10, NONE);
    
    /* keyboard scanner is a timmer */
    sys_vec_set(rst38_handler,RST38); 

    /* clear screen and set border and paper */
    tty_cls();

    /* goto 0,0 */
    tty_xy(0,31);
    tty_printf("XYZ OS 0.2 (c) 2021 TOMAZ STIH\n\n");

    /* register syscalls (api) */
    yos_t* y=_yos_init();
    svc_register("yos",y);

    /* and run shell */
    shell();
}