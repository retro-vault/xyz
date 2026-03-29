/*
 * test.c
 *
 * test (from yos shell)
 *
 * this executes current test (not part of the OS)
 *
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-07-18   tstih
 *
 */
#include <stdbool.h>

#include <kernel/list.h>
#include <kernel/mem.h>
#include <kernel/service.h>
#include <kernel/thread.h>

#include <yos.h>

#ifdef HAVE_GPX
#include <gpx.h>
#endif

extern yos_t *y;

thread_t *t1;
thread_t *t2;

void tower(int column) {
    column;
    __asm
    ;; get column to de
    pop     hl
    pop     de
    push    de
    push    hl
    ld      hl,#0x4000
    add     hl,de                       ; correct address
    ld      b,#100
tow_loop:
    ld      (hl),#0xff
    call    tv_nextrow
    djnz    tow_loop
    __endasm;
}

void thread1(void) {
    tower(26);
}

void thread2(void) {
    tower(30);
}

void test_threads(void) {

    y->setcur(false);
    y->clrscr();
    y->printf("\nCREATED TWO THREADS\n\n");
    y->printf("Press enter to abort...\n");

    t1=thread_create(thread1, 512, thread_current->process);
    thread_resume(t1);
    t2=thread_create(thread2, 512, thread_current->process);
    thread_resume(t2);

    while (!y->kbhit());
    y->setcur(true);

    thread_exit(t1);
    thread_suspend(t2);
}

#ifdef HAVE_GPX

#define printf y->printf

void test_gpx(void) {

    gpx_t *g=gpx_init();
    gpx_cls(g);

    gpx_cap_t *cap=gpx_cap(g);
    coord centerx = cap->pages[0].resolutions[0].width/2;
    coord centery = cap->pages[0].resolutions[0].height/2;

    printf("Center is at %d,%d\n",centerx, centery);

    for (coord x=centerx-20; x<centerx+20;x++)
        gpx_draw_pixel(g,x,centery);

    gpx_draw_circle(g,centerx,centery,20);
    gpx_exit(NULL);
}

#undef printf

#endif /* HAVE_GPX */

void _test(void) {
#ifdef HAVE_GPX
    test_gpx();
#else
    test_threads();
#endif
}
