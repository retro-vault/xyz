/*
 *  kernel.c
 *  yos kernel (main!)
 *
 *  tomaz stih dec 18 2020
 */
#include "console.h"

void main() {
    con_clrscr();
    con_puts("the yos operating system 0.1\n");
    con_puts("for zx spectrum 48k\n");
    con_puts("(c) 2020 tomaz stih\n");
    con_puts("\n");
    con_puts("ready?");
}