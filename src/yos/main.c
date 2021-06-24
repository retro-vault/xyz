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

/* must be naked, returns with reti */
void kbd_handler() __naked {
    __asm
        call    _ir_disable
        ;; store all registers
        push    af
        push    bc
        push    de
        push    hl 
        ;; scan keyboard
        call    _kbd_scan
        ;; restore regs and allow interrupt again
        pop     hl
        pop     de
        pop     bc
        pop     af
        call    _ir_enable
        reti
    __endasm;
}

void main() {

    /* create system and user heap  */
    mem_init((void *)&sys_heap,1024);
    mem_init((void *)&heap,0xffff-&sys_heap);

    /* install timer */
    
    /* keyboard scanner is a timmer */
    sys_vec_set(kbd_handler,RST38); 

    /* clear screen and set border and paper */
    tty_cls();

    /* goto 0,0 */
    tty_xy(0,31);
    printf("XYZ OS 0.1 (c) 2021 TOMAZ STIH\n\n");
    printf(
        "SYS HEAP: %04x  USR HEAP: %04x  FREE: %02dKB\n\nREADY?", 
        &sys_heap, 
        &heap,
        ((0xffff-&heap)/1024)
    );

    /* we'll print this */
    char c;
    while (true) 
        if (c=tty_getc()) tty_putc(c);
        
}