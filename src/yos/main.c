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
#include <ctype.h>

#include <kernel/vectors.h>
#include <kernel/mem.h>
#include <kernel/timer.h>
#include <kernel/service.h>
#include <kernel/thread.h>
#include <kernel/process.h>

#include <drivers/kbd.h>
#include <drivers/time.h>

#include <tty/tty.h>
#include <tty/tty_print.h>

#include <yos.h>

extern yos_t* _yos_init();
extern void _clock_tick();
extern void ysh(); /* yos shell */

void main() {

    /* create system and user heap  */
    mem_init((void *)&_sys_heap,1024);
    mem_init((void *)&_heap,0xffff-&_sys_heap);

    /* install cursor, keyboard, and
       real time clock timers*/
    tmr_install(_tty_cur_tick, 10, NONE);
    tmr_install(_kbd_scan, 0, NONE);
    tmr_install(_clock_tick, 0, NONE);

    /* clear screen and set border and paper */
    tty_cls();

    /* goto 0,0 */
    tty_xy(0,31);
    tty_printf("XYZ OS 0.2 (c) 2021 TOMAZ STIH\n\n");

    /* register syscalls (api) service */
    yos_t* y=_yos_init();
    svc_register("yos",y);

    /* create shell process */
    process_t *p=process_start("ysh", ysh, 1024);

    /* and yield control to the scheduler */
    sys_vec_set(_thread_robin,RST38); 
}

