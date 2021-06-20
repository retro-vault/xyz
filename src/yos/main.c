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

void main() {
    /* TODO: Install keyboard scanner.
       sys_vec_set(handler,RST38); */

    /* Clear screen and set border and paper. */
    tty_cls();

    /* Fill the screen with letters. */
    for(int x=0;x<42;x++)
        for(int y=0;y<32;y++) {
            tty_xy(x,y);
            tty_putc('A'+y);
        }

    /* let's try to scan keyboard without a vector */
    bool kbhit=false;
    while (!kbhit) {
        kbd_scan();
        kbhit=kbd_read();
    }

     /* Scroll them all off the screen. */
    for (int y=0;y<32;y++)
        tty_scroll();
}