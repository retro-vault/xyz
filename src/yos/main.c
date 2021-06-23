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
#include <vectors.h>
#include <kbd.h>
#include <tty.h>

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
    /* install keyboard scanner */
    sys_vec_set(kbd_handler,RST38); 

    /* clear screen and set border and paper */
    tty_cls();

    /* fill the screen with letters */
    for(int x=0;x<42;x++)
        for(int y=0;y<32;y++) {
            tty_xy(x,y);
            tty_putc('A'+y);
        }

    /* wait for keyboard */
    while (!kbd_read());

     /* scroll them all off the screen */
    for (int y=0;y<32;y++)
        tty_scroll();

    /* goto 0,0 */
    tty_xy(0,31);
    tty_puts("XYZ OS 0.1\n(c) 2021 TOMAZ STIH\n\nREADY?");

    /* we'll print this */
    char c, s[]={0,0};
    while (true) {
        c=tty_getc();
        if (c) {
            s[0]=c;
            tty_puts(s);
        }
    }
}