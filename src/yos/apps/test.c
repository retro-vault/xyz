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
#include <gpx.h>

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

void thread1() {
    tower(26);
}

void thread2() {
    tower(30);
}

void test_threads() {

     /* welcome msg */
    y->setcur(false);
    y->clrscr();
    y->printf("\nCREATED TWO THREADS\n\n");
    y->printf("Press enter to abort...\n");
    /* create threads */
    t1=thread_create(thread1, 512, thread_current->process);
    thread_resume(t1);
    t2=thread_create(thread2, 512, thread_current->process);
    thread_resume(t2);
    /* and wait */
    while (!y->kbhit());
    y->setcur(true);
    /* suspend one thread */
    thread_exit(t1);
    thread_suspend(t2);

}

#define printf y->printf

void test_gpx() {

    printf("\n\n");

    /* enter graphics */
    gpx_t *g=gpx_init();

    /* query graphics capabilities */
    gpx_cap_t *cap=gpx_cap(g);
    printf("GRAPHICS PROPERTIES\n\n");
    printf("No. colors %d\nBack color %d\nFore color %d\n",
        cap->num_colors,
        cap->back_color,
        cap->fore_color);
    printf("Sup. pages %d\n", cap->num_pages);
    for(int p=0; p<cap->num_pages; p++)
        for (int r=0; r<cap->pages[p].num_resolutions; r++)
            printf(" P%d Resol. %dx%d\n",
                p,
                cap->pages[p].resolutions[r].width,
                cap->pages[p].resolutions[r].height);


    /* leave graphics */
    gpx_exit(NULL);


    printf("\n\n");
}

#undef printf

void _test() {
    test_gpx();
}